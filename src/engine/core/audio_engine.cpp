#include "engine/core/audio_engine.hpp"
#include "engine/nodes/flux_track_node.hpp"
#include "engine/nodes/input_node.hpp"
#include "engine/dsp/flux_audio_utils.hpp"
#include "engine/dsp/garbage_collector.hpp"
#include <iostream>
#include <cstring>
#include <vector>

namespace Beam {

AudioEngine::AudioEngine() : m_sampleRate(44100), m_channels(2), m_blockSize(512) {
    m_activePlan.store(nullptr);
    m_scratchBuffer.resize(1024 * 2, 0.0f); // Pre-allocate safe default
}

AudioEngine::~AudioEngine() {
}

// Unified Audio Callback from Device Manager
void AudioEngine::audioCallback(const std::map<std::string, float*>& inputs, float** outputs, int frames, int inChannels, int outChannels, double sampleRate) {
    // 1. Distribute Hardware Inputs to InputNodes
    if (m_graph) {
        auto nodes = m_graph->getNodes();
        for (auto& [id, node] : nodes) {
            auto inputNode = std::dynamic_pointer_cast<InputNode>(node);
            if (inputNode) {
                auto deviceId = inputNode->getDeviceId();
                if (inputs.count(deviceId)) {
                    float* inPtr = inputs.at(deviceId);
                    inputNode->pushData(inPtr, frames * 2);
                }
            }
        }
    }

    // 2. Process DSP directly to hardware output buffer pointer
    outputs[0] = process(frames);
}

bool AudioEngine::init(int sampleRate, int channels, int blockSize, const std::string& outputDevice, const std::string& inputDevice) {
    m_isMuted = true;

    m_sampleRate = sampleRate;
    m_channels = channels;
    m_blockSize = blockSize;

    if (!m_masterNode) m_masterNode = std::make_shared<MasterNode>(blockSize * 4);
    
    if (!m_inputNode) {
        m_inputNode = std::make_shared<InputNode>(blockSize * 4);
    }
    
    if (!inputDevice.empty()) {
        m_inputNode->setDeviceId(inputDevice);
    }
    
    updatePlan();
    
    m_isMuted = false;
    return true;
}

void AudioEngine::setGraph(std::shared_ptr<FluxGraph> graph) {
    if (m_graph == graph) return;
    m_graph = graph;
    if (m_graph) {
        m_masterNodeId = m_graph->addNode(m_masterNode);
        updatePlan();
    }
}

void AudioEngine::setMasterNodeId(size_t id) { m_masterNodeId = id; }

void AudioEngine::updatePlan() {
    if (m_graph) {
        auto oldPlan = m_activePlan.load();
        auto newPlan = m_graph->compile(m_blockSize, m_channels, m_masterNodeId, &m_mixerState);
        m_activePlan.store(newPlan);
        
        size_t maxRequired = m_blockSize * m_channels;
        if (m_scratchBuffer.size() < maxRequired) {
            m_scratchBuffer.resize(maxRequired);
        }
        
        if (oldPlan) {
            GarbageCollector::get().defer(oldPlan);
        }
    }
}

void AudioEngine::setPlaying(bool playing) { m_isPlaying = playing; }
void AudioEngine::rewind() { seek(0); }
void AudioEngine::seek(size_t frame) { m_pendingSeek.store((int64_t)frame); }
    
void AudioEngine::setMuted(bool muted) { 
    m_isMuted.store(muted); 
    if (muted) {
        // Wait for audio thread to exit process() to ensure exclusive access to VSTs
        int timeout = 100; // 100ms max wait
        while (m_isProcessing.load() && timeout-- > 0) {
            SDL_Delay(1);
        }
    }
}

float* AudioEngine::process(int frames) {
    m_isProcessing.store(true);

    if (m_isMuted.load(std::memory_order_relaxed)) {
        SIMD::set(m_scratchBuffer.data(), 0.0f, frames * m_channels);
        m_isProcessing.store(false);
        return m_scratchBuffer.data();
    }

    std::shared_ptr<RenderPlan> plan = m_activePlan.load();
    if (!plan) {
        SIMD::set(m_scratchBuffer.data(), 0.0f, frames * m_channels);
        m_isProcessing.store(false);
        return m_scratchBuffer.data();
    }

    bool currentPlaying = m_isPlaying.load();
    if (currentPlaying != m_lastPlaying) {
        for (auto& node : plan->allNodes) node->onTransportStateChanged(currentPlaying);
        m_lastPlaying = currentPlaying;
    }

    int64_t target = m_pendingSeek.exchange(-1);
    if (target != -1) {
        m_currentFrame.store((size_t)target);
        for (auto& node : plan->allNodes) node->onTransportSeek((size_t)target);
    }

    size_t current = m_currentFrame.load();

    if (currentPlaying) {
        std::unique_lock<std::mutex> lock(m_automationMutex, std::try_to_lock);
        if (lock.owns_lock()) {
            for (auto& lane : m_automationLanes) {
                if (lane->isRecording()) {
                    if (auto p = lane->getParameter()) lane->recordPoint(current, p->getValue());
                } else {
                    lane->applyAt(current);
                }
            }
        }
    }

    if (plan) {
        // --- Mixer Global Pre-scan ---
        bool anySolo = false;
        for (auto const& execScan : plan->sequence) {
            for (auto const& routeScan : execScan.outgoingRoutes) {
                if (routeScan.soloPtr && routeScan.soloPtr->load(std::memory_order_relaxed)) {
                    anySolo = true;
                    break;
                }
            }
            if (anySolo) break;
        }

        for (int bIdx : plan->clearBufferIndices) {
            if (bIdx >= 0 && bIdx < (int)plan->bufferPool.size()) {
                auto& buf = plan->bufferPool[bIdx];
                std::fill(buf.begin(), buf.end(), 0.0f);
            }
        }

        static float paramCache[4096]; // Increased for safety

        for (auto& exec : plan->sequence) {
            if (exec.processor) exec.processor->setCurrentFrame(current);

            size_t pCount = exec.parameterPointers.size();
            for (size_t i = 0; i < pCount && i < 4096; ++i) {
                if (exec.parameterPointers[i])
                    paramCache[i] = exec.parameterPointers[i]->load(std::memory_order_relaxed);
                else
                    paramCache[i] = 0.0f;
            }
            exec.processor->updateParameters(paramCache);

            const float* inputs[8]; 
            float* outputs[8];
            
            for (int i = 0; i < 8; ++i) {
                if (i < (int)exec.inputBufferIndices.size()) {
                    int idx = exec.inputBufferIndices[i];
                    inputs[i] = (idx >= 0 && idx < (int)plan->bufferPool.size()) ? plan->bufferPool[idx].data() : nullptr;
                } else {
                    inputs[i] = nullptr;
                }
            }
            
            for (int i = 0; i < 8; ++i) {
                if (i < (int)exec.outputBufferIndices.size()) {
                    int idx = exec.outputBufferIndices[i];
                    outputs[i] = (idx >= 0 && idx < (int)plan->bufferPool.size()) ? plan->bufferPool[idx].data() : nullptr;
                } else {
                    outputs[i] = nullptr;
                }
            }

            if (exec.processor) exec.processor->process(inputs, outputs, frames);

            for (auto& route : exec.outgoingRoutes) {
                bool isMuted = route.mutePtr && route.mutePtr->load(std::memory_order_relaxed);
                bool isSoloed = route.soloPtr && route.soloPtr->load(std::memory_order_relaxed);
                
                // If any channel is soloed, this channel is audible ONLY if it is also soloed.
                // Otherwise (no solo active), it is audible if not muted.
                bool isAudible = anySolo ? isSoloed : !isMuted;

                if (!isAudible) {
                    if (route.peakPtr) route.peakPtr->store(0.0f, std::memory_order_relaxed);
                    continue; 
                }
                
                float gain = route.gainPtr ? route.gainPtr->load(std::memory_order_relaxed) : 1.0f;
                float pan = route.panPtr ? route.panPtr->load(std::memory_order_relaxed) : 0.5f;
                
                int srcIdx = route.sourceBufferIdx;
                int dstIdx = route.destBufferIdx;

                if (srcIdx < 0 || srcIdx >= (int)plan->bufferPool.size() || 
                    dstIdx < 0 || dstIdx >= (int)plan->bufferPool.size()) continue;

                float* src = plan->bufferPool[srcIdx].data();
                float* dst = plan->bufferPool[dstIdx].data();
                int total = frames * m_channels;
                
                if (route.peakPtr) {
                    float peak = 0.0f;
                    for (int s = 0; s < total; s+=4) {
                        float absVal = std::abs(src[s] * gain);
                        if (absVal > peak) peak = absVal;
                    }
                    route.peakPtr->store(peak, std::memory_order_relaxed);
                }
                
                // Apply gain and constant-power panning
                float p = pan * 1.570796f;
                float panL = std::cos(p);
                float panR = std::sin(p);

                SIMD::add_with_gain_pan(src, dst, gain, panL, panR, frames);
            }
        }

        if (currentPlaying) m_currentFrame.fetch_add(frames);

        if (!plan->masterOutputBufferIndices.empty()) {
            int leftIdx = plan->masterOutputBufferIndices[0];
            m_isProcessing.store(false);
            return plan->bufferPool[leftIdx].data();
        }
    }

    SIMD::set(m_scratchBuffer.data(), 0.0f, frames * m_channels);
    m_isProcessing.store(false);
    return m_scratchBuffer.data();
}

} // namespace Beam