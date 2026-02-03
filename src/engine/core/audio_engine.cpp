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
    
float* AudioEngine::process(int frames) {
    if (m_isMuted.load(std::memory_order_relaxed)) {
        SIMD::set(m_scratchBuffer.data(), 0.0f, frames * m_channels);
        return m_scratchBuffer.data();
    }

    std::shared_ptr<RenderPlan> plan = m_activePlan.load();
    if (!plan) {
        SIMD::set(m_scratchBuffer.data(), 0.0f, frames * m_channels);
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
        for (int bIdx : plan->clearBufferIndices) {
            auto& buf = plan->bufferPool[bIdx];
            std::fill(buf.begin(), buf.end(), 0.0f);
        }

        float paramCache[64]; 

        for (auto& exec : plan->sequence) {
            int pCount = (int)exec.parameterPointers.size();
            for (int i = 0; i < pCount && i < 64; ++i) {
                paramCache[i] = exec.parameterPointers[i]->load(std::memory_order_relaxed);
            }
            exec.processor->updateParameters(paramCache);

            const float* inputs[8]; 
            float* outputs[8];
            
            for (int i = 0; i < 8; ++i) {
                if (i < (int)exec.inputBufferIndices.size()) {
                    inputs[i] = plan->bufferPool[exec.inputBufferIndices[i]].data();
                } else {
                    inputs[i] = nullptr;
                }
            }
            
            for (int i = 0; i < (int)exec.outputBufferIndices.size() && i < 8; ++i)
                outputs[i] = plan->bufferPool[exec.outputBufferIndices[i]].data();

            exec.processor->process(inputs, outputs, frames);

            for (auto& route : exec.outgoingRoutes) {
                if (route.mutePtr && route.mutePtr->load(std::memory_order_relaxed)) {
                    if (route.peakPtr) route.peakPtr->store(0.0f, std::memory_order_relaxed);
                    continue; 
                }
                
                float gain = route.gainPtr ? route.gainPtr->load(std::memory_order_relaxed) : 1.0f;
                float* src = plan->bufferPool[route.sourceBufferIdx].data();
                float* dst = plan->bufferPool[route.destBufferIdx].data();
                int total = frames * m_channels;
                
                if (route.peakPtr) {
                    float peak = 0.0f;
                    for (int s = 0; s < total; s+=4) {
                        float absVal = std::abs(src[s] * gain);
                        if (absVal > peak) peak = absVal;
                    }
                    route.peakPtr->store(peak, std::memory_order_relaxed);
                }
                
                if (gain == 1.0f) {
                    SIMD::add(dst, src, total);
                } else if (gain > 0.0f) {
                    SIMD::add_with_gain(src, dst, gain, total);
                }
            }
        }

        if (currentPlaying) m_currentFrame.fetch_add(frames);

        if (!plan->masterOutputBufferIndices.empty()) {
            int leftIdx = plan->masterOutputBufferIndices[0];
            return plan->bufferPool[leftIdx].data();
        }
    }

    SIMD::set(m_scratchBuffer.data(), 0.0f, frames * m_channels);
    return m_scratchBuffer.data();
}

} // namespace Beam