#include "audio_engine.hpp"
#include "flux_track_node.hpp"
#include <iostream>

namespace Beam {

AudioEngine::AudioEngine() : m_sampleRate(44100), m_channels(2), m_stream(nullptr) {
    // Initialize atomic shared_ptr to null
    std::atomic_store(&m_activePlan, std::shared_ptr<RenderPlan>(nullptr));
}

AudioEngine::~AudioEngine() {
    if (m_stream) SDL_DestroyAudioStream(m_stream);
}

bool AudioEngine::init(int sampleRate, int channels) {
    m_sampleRate = sampleRate;
    m_channels = channels;

    m_masterNode = std::make_shared<MasterNode>(1024 * 4);
    
    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_F32;
    spec.channels = channels;
    spec.freq = sampleRate;

    m_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!m_stream) {
        const char* err = SDL_GetError();
        std::cerr << "CRITICAL: SDL_OpenAudioDeviceStream failed: " << err << std::endl;
        return false;
    }

    SDL_ResumeAudioStreamDevice(m_stream);
    return true;
}

void AudioEngine::setGraph(std::shared_ptr<FluxGraph> graph) {
    // UI Thread operation
    if (m_graph == graph) return; // No change
    
    m_graph = graph;
    if (m_graph) {
        // Only add master node if we haven't already linked it to this graph
        // In a more robust system, we'd check if graph contains m_masterNode
        m_masterNodeId = m_graph->addNode(m_masterNode);
        updatePlan();
    }
}

void AudioEngine::updatePlan() {
    if (m_graph) {
        auto newPlan = m_graph->compile(1024, m_channels);
        std::atomic_store(&m_activePlan, newPlan);
    }
}

void AudioEngine::setPlaying(bool playing) {
    m_isPlaying = playing;
    // Notify graph model (UI side)
    if (m_graph) {
        m_graph->setTransportState(playing);
    }
    // Note: We don't necessarily need to update the plan just for transport state 
    // if the nodes read transport state from a shared atomic or if they are updated 
    // via direct method calls. Ideally, Transport Info should be passed in process(),
    // but for now, the existing FluxNode::onTransportStateChanged is okay.
}

void AudioEngine::rewind() {
    if (m_graph) {
        // Iterate through all nodes and notify seek
        // This is safe-ish if nodes handle seek atomically or if we are paused.
        for (auto const& [id, node] : m_graph->getNodes()) {
            node->onTransportSeek(0);
        }
    }
}
    
void AudioEngine::process(float* output, int frames) {
    if (!m_isPlaying || !m_stream) {
        return;
    }

    // Prevent buffer overflow / high latency
    const int MAX_QUEUED_BYTES = 20000; 
    if (SDL_GetAudioStreamQueued(m_stream) > MAX_QUEUED_BYTES) {
        return;
    }

    // LOCK-FREE EXECUTION
    // 1. Atomically load the current plan
    std::shared_ptr<RenderPlan> plan = std::atomic_load(&m_activePlan);

    if (plan) {
        // 2. Clear buffers
        for (const auto& op : plan->clearOps) {
            std::fill(op.buffer, op.buffer + op.size, 0.0f);
        }

        // 3. Execute Nodes & Route Signal
        for (const auto& exec : plan->sequence) {
            if (!exec.node->isBypassed()) {
                exec.node->process(frames);
            } else {
                 // Clear outputs if bypassed
                for (int i = 0; i < exec.node->getOutputPorts().size(); ++i) {
                    float* buf = exec.node->getOutputBuffer(i);
                    std::fill(buf, buf + frames * 2, 0.0f);
                }
            }

            // Route outputs to inputs
            for (const auto& route : exec.outgoingRoutes) {
                // Summing into destination
                // Use explicit loop for now, SIMD later
                // Assuming route.sourceBuffer and route.destBuffer are valid
                // and size is frames * channels
                int totalSamples = frames * m_channels;
                for (int i = 0; i < totalSamples; ++i) {
                    route.destBuffer[i] += route.sourceBuffer[i];
                }
            }
        }
    }

    // 4. Copy Master Output
    // We access m_masterNode directly here. In a pure system, MasterNode 
    // would also be part of the plan, but we keep a handle to it for convenience.
    // Ensure m_masterNode is thread-safe or only accessed here.
    float* masterIn = m_masterNode->getInputBuffer(0);
    
    // Copy to SDL buffer
    for (int i = 0; i < frames * m_channels; ++i) {
        output[i] = masterIn[i];
    }

    // Push to SDL Stream
    SDL_PutAudioStreamData(m_stream, output, frames * m_channels * sizeof(float));
}

} // namespace Beam