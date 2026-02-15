#include "engine/core/audio_engine.hpp"
#include "engine/nodes/flux_track_node.hpp"
#include "engine/nodes/input_node.hpp"
#include "engine/dsp/flux_audio_utils.hpp"
#include "engine/dsp/garbage_collector.hpp"
#include "engine/core/thread_pool.hpp"
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
    // 1. Distribute Hardware Inputs to InputNodes via the active plan (thread-safe)
    auto plan = m_activePlan.load();
    if (plan) {
        for (auto& node : plan->allNodes) {
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

    // Initialize/Resize caches
    m_paramCache.resize(4096, 0.0f);
    m_inputPtrCache.resize(64, nullptr); // Support up to 64 I/O for now, dynamic resize in process if needed
    m_outputPtrCache.resize(64, nullptr);

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
        
        // Snapshot active automation lanes into the plan
        {
            std::lock_guard<std::mutex> lock(m_automationMutex);
            newPlan->activeAutomationLanes = m_automationLanes;
        }

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
    // Non-blocking mute. 
    // We rely on the atomic store and the check in process()
}

float* AudioEngine::process(int frames) {
    m_isProcessing.store(true);

    if (m_isMuted.load(std::memory_order_relaxed)) {
        SIMD::set(m_scratchBuffer.data(), 0.0f, frames * m_channels);
        m_isProcessing.store(false);
        return m_scratchBuffer.data();
    }

    std::shared_ptr<RenderPlan> plan = m_activePlan.load();
    if (!plan || plan->levels.empty()) {
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
        for (auto& lane : plan->activeAutomationLanes) {
            if (lane->isRecording()) {
                if (auto p = lane->getParameter()) lane->recordPoint(current, p->getValue());
            } else {
                lane->applyAt(current);
            }
        }
    }

    // --- Mixer Global Pre-scan (Solo Logic) ---
    bool anySolo = false;
    for (auto const& level : plan->levels) {
        for (auto const& exec : level) {
            for (auto const& route : exec.outgoingRoutes) {
                if (route.soloPtr && route.soloPtr->load(std::memory_order_relaxed)) {
                    anySolo = true;
                    break;
                }
            }
            if (anySolo) break;
        }
        if (anySolo) break;
    }

    // --- Clear Buffers (Serial) ---
    for (int bIdx : plan->clearBufferIndices) {
        if (bIdx >= 0 && bIdx < (int)plan->bufferPool.size()) {
            auto& buf = plan->bufferPool[bIdx];
            std::fill(buf.begin(), buf.end(), 0.0f);
        }
    }

    // --- Prepare Parameter Ramps (Serial) ---
    for (auto& level : plan->levels) {
        for (auto& exec : level) {
            for (auto* p : exec.parameters) {
                if (p) p->prepareRamp(frames);
            }
        }
    }

    // --- Execute Levels in Sequence ---
    for (auto& level : plan->levels) {
        // 1. Parallel Processor Execution
        ThreadPool::get().parallelFor(level, [&](ProcessorExecution& exec) {
            if (!exec.processor) return;
            
            exec.processor->setCurrentFrame(current);

            // Stack-allocated caches for thread-safety (limit to 128 for stack safety)
            float localParamCache[128]; 
            const float* localInputs[64];
            float* localOutputs[64];

            size_t pCount = exec.parameterPointers.size();
            for (size_t i = 0; i < pCount && i < 128; ++i) {
                if (exec.parameterPointers[i])
                    localParamCache[i] = exec.parameterPointers[i]->load(std::memory_order_relaxed);
                else
                    localParamCache[i] = 0.0f;
            }
            exec.processor->updateParameters(localParamCache);

            size_t inCount = exec.inputBufferIndices.size();
            size_t outCount = exec.outputBufferIndices.size();
            
            for (size_t i = 0; i < inCount && i < 64; ++i) {
                int idx = exec.inputBufferIndices[i];
                localInputs[i] = (idx >= 0 && idx < (int)plan->bufferPool.size()) ? plan->bufferPool[idx].data() : nullptr;
            }
            
            for (size_t i = 0; i < outCount && i < 64; ++i) {
                int idx = exec.outputBufferIndices[i];
                localOutputs[i] = (idx >= 0 && idx < (int)plan->bufferPool.size()) ? plan->bufferPool[idx].data() : nullptr;
            }

            // Check bypass state - if bypassed, just copy input to output (passthrough)
            bool bypassed = exec.bypassPtr && exec.bypassPtr->load(std::memory_order_relaxed);
            if (bypassed) {
                // Bypass: copy first input to all outputs (common mono/stereo passthrough)
                for (size_t i = 0; i < outCount && i < 64; ++i) {
                    if (localOutputs[i] && inCount > 0 && localInputs[0]) {
                        size_t inIdx = i < inCount ? i : 0;  // Use corresponding input if available, else first
                        if (localInputs[inIdx]) {
                            memcpy(localOutputs[i], localInputs[inIdx], frames * m_channels * sizeof(float));
                        }
                    }
                }
            } else {
                exec.processor->process((const float**)localInputs, localOutputs, frames);
            }
        });

        // 2. Serial Routing (Summation)
        // This MUST be serial because multiple level nodes can write to the same destination buffer
        for (auto& exec : level) {
            for (auto& route : exec.outgoingRoutes) {
                bool isMuted = route.mutePtr && route.mutePtr->load(std::memory_order_relaxed);
                bool isSoloed = route.soloPtr && route.soloPtr->load(std::memory_order_relaxed);
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
                
                if (route.peakPtr) {
                    float peak = 0.0f;
                    for (int s = 0; s < frames * m_channels; s+=4) {
                        float absVal = std::abs(src[s] * gain);
                        if (absVal > peak) peak = absVal;
                    }
                    route.peakPtr->store(peak, std::memory_order_relaxed);
                }
                
                float p = pan * 1.570796f;
                float panL = std::cos(p);
                float panR = std::sin(p);
                SIMD::add_with_gain_pan(src, dst, gain, panL, panR, frames);
            }
        }
    }

    if (currentPlaying) m_currentFrame.fetch_add(frames);

    if (!plan->masterOutputBufferIndices.empty()) {
        int leftIdx = plan->masterOutputBufferIndices[0];
        m_isProcessing.store(false);
        return plan->bufferPool[leftIdx].data();
    }

    SIMD::set(m_scratchBuffer.data(), 0.0f, frames * m_channels);
    m_isProcessing.store(false);
    return m_scratchBuffer.data();
}

} // namespace Beam