#include "engine/core/audio_engine.hpp"
#include "engine/nodes/flux_track_node.hpp"
#include "engine/nodes/input_node.hpp"
#include "engine/dsp/simd_utils.hpp"
#include "engine/dsp/garbage_collector.hpp"
#include <iostream>
#include <cstring>
#include <vector>

namespace Beam {

AudioEngine::AudioEngine() : m_sampleRate(44100), m_channels(2) {
    m_activePlan.store(nullptr);
}

AudioEngine::~AudioEngine() {
}

// Unified Audio Callback from Device Manager
void AudioEngine::audioCallback(const std::map<std::string, float*>& inputs, float** output, int frames, int inChannels, int outChannels, double sampleRate) {
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

    // 2. Process DSP
    std::vector<float> tempBuf(frames * outChannels);
    process(tempBuf.data(), frames);
    
    // 3. Copy to Hardware Out
    memcpy(output[0], tempBuf.data(), frames * outChannels * sizeof(float));
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
    
    // Always update device ID if provided
    if (!inputDevice.empty()) {
        m_inputNode->setDeviceId(inputDevice);
    } else if (m_inputNode->getDeviceId().empty()) {
        m_inputNode->setDeviceId(""); // Explicit default
    }
    
    // Recompile plan to match new block size/rate
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

void AudioEngine::updatePlan() {
    if (m_graph) {
        auto oldPlan = m_activePlan.load();
        auto newPlan = m_graph->compile(m_blockSize, m_channels, m_masterNodeId);
        m_activePlan.store(newPlan);
        
        if (oldPlan) {
            GarbageCollector::get().defer(oldPlan);
        }
    }
}

void AudioEngine::setPlaying(bool playing) { m_isPlaying = playing; }
void AudioEngine::rewind() { seek(0); }
void AudioEngine::seek(size_t frame) { m_pendingSeek.store((int64_t)frame); }
    
void AudioEngine::process(float* output, int frames) {
    if (m_isMuted.load(std::memory_order_relaxed)) {
        std::fill(output, output + frames * m_channels, 0.0f);
        return;
    }

    std::shared_ptr<RenderPlan> plan = m_activePlan.load();

    // 1. Handle Transport State Transition
    bool currentPlaying = m_isPlaying.load();
    if (currentPlaying != m_lastPlaying) {
        if (plan) {
            for (auto& node : plan->allNodes) node->onTransportStateChanged(currentPlaying);
        }
        m_lastPlaying = currentPlaying;
    }

    // 2. Handle Pending Seek
    int64_t target = m_pendingSeek.exchange(-1);
    if (target != -1) {
        m_currentFrame.store((size_t)target);
        if (plan) {
            for (auto& node : plan->allNodes) node->onTransportSeek((size_t)target);
        }
    }

    size_t current = m_currentFrame.load();
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
                    // Always provide the buffer pointer. It's guaranteed to exist 
                    // and be zeroed by the plan if no signal is routed to it.
                    inputs[i] = plan->bufferPool[exec.inputBufferIndices[i]].data();
                } else {
                    inputs[i] = nullptr;
                }
            }
            
            for (int i = 0; i < (int)exec.outputBufferIndices.size() && i < 8; ++i)
                outputs[i] = plan->bufferPool[exec.outputBufferIndices[i]].data();

            exec.processor->process(inputs, outputs, frames);

            for (auto& route : exec.outgoingRoutes) {
                SIMD::add(plan->bufferPool[route.sourceBufferIdx].data(),
                          plan->bufferPool[route.destBufferIdx].data(),
                          frames * m_channels);
            }
        }

        if (!plan->masterOutputBufferIndices.empty()) {
            int leftIdx = plan->masterOutputBufferIndices[0];
            SIMD::copy(plan->bufferPool[leftIdx].data(), output, frames * m_channels);
        } else {
            std::fill(output, output + frames * m_channels, 0.0f);
        }
    }

    if (currentPlaying) {
        m_currentFrame.fetch_add(frames);
    }
}

} // namespace Beam
