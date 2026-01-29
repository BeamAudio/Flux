#include "audio_engine.hpp"
#include "flux_track_node.hpp"
#include "input_node.hpp"
#include "simd_utils.hpp"
#include <iostream>

namespace Beam {

AudioEngine::AudioEngine() : m_sampleRate(44100), m_channels(2), m_stream(nullptr), m_captureStream(nullptr) {
    m_activePlan.store(nullptr);
}

AudioEngine::~AudioEngine() {
    if (m_stream) SDL_DestroyAudioStream(m_stream);
    if (m_captureStream) SDL_DestroyAudioStream(m_captureStream);
}

bool AudioEngine::init(int sampleRate, int channels) {
    m_sampleRate = sampleRate;
    m_channels = channels;

    m_masterNode = std::make_shared<MasterNode>(1024 * 4);
    m_inputNode = std::make_shared<InputNode>(1024 * 4);
    
    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_F32;
    spec.channels = channels;
    spec.freq = sampleRate;

    // Output Stream
    m_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!m_stream) {
        std::cerr << "CRITICAL: SDL_OpenAudioDeviceStream (Playback) failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Input Stream
    m_captureStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_RECORDING, &spec, NULL, NULL);
    if (!m_captureStream) {
        std::cerr << "WARNING: SDL_OpenAudioDeviceStream (Recording) failed: " << SDL_GetError() << std::endl;
        // We can continue without recording
    } else {
        SDL_ResumeAudioStreamDevice(m_captureStream);
    }

    SDL_ResumeAudioStreamDevice(m_stream);
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
        auto newPlan = m_graph->compile(1024, m_channels);
        m_activePlan.store(newPlan);
    }
}

void AudioEngine::setPlaying(bool playing) {
    m_isPlaying = playing;
    if (m_graph) {
        m_graph->setTransportState(playing);
    }
}

void AudioEngine::rewind() {
    seek(0);
}

void AudioEngine::seek(size_t frame) {
    if (m_graph) {
        m_currentFrame = frame;
        for (auto const& [id, node] : m_graph->getNodes()) {
            node->onTransportSeek(frame);
        }
    }
}
    
void AudioEngine::process(float* output, int frames, const MIDIBuffer& midi) {
    // 1. Capture Input
    if (m_captureStream && m_inputNode) {
        int available = SDL_GetAudioStreamAvailable(m_captureStream);
        if (available > 0) {
            std::vector<float> captureBuf(available / sizeof(float));
            SDL_GetAudioStreamData(m_captureStream, captureBuf.data(), available);
            m_inputNode->pushData(captureBuf.data(), (int)captureBuf.size());
        }
    }

    if (!m_isPlaying || !m_stream) {
        return;
    }

    for (auto& lane : m_automationLanes) {
        lane->applyAt(m_currentFrame);
    }

    const int MAX_QUEUED_BYTES = 20000; 
    if (SDL_GetAudioStreamQueued(m_stream) > MAX_QUEUED_BYTES) {
        return;
    }

    std::shared_ptr<RenderPlan> plan = m_activePlan.load();

    if (plan) {
        for (const auto& op : plan->clearOps) {
            float* buf = op.node->getInputBuffer(op.portIdx);
            std::fill(buf, buf + frames * m_channels, 0.0f);
        }

        for (const auto& exec : plan->sequence) {
            exec.node->setCurrentFrame(m_currentFrame);
            
            if (!midi.getEvents().empty()) {
                exec.node->processMIDI(midi);
            }

            if (!exec.node->isBypassed()) {
                exec.node->process(frames);
            } else {
                for (int i = 0; i < (int)exec.node->getOutputPorts().size(); ++i) {
                    float* buf = exec.node->getOutputBuffer(i);
                    std::fill(buf, buf + frames * m_channels, 0.0f);
                }
            }

            for (const auto& route : exec.outgoingRoutes) {
                float* src = route.sourceNode->getOutputBuffer(route.sourcePort);
                float* dst = route.destNode->getInputBuffer(route.destPort);
                SIMD::add(src, dst, frames * m_channels);
            }
        }
    }

    float* masterIn = m_masterNode->getInputBuffer(0);
    SIMD::copy(masterIn, output, frames * m_channels);

    SDL_PutAudioStreamData(m_stream, output, frames * m_channels * sizeof(float));

    m_currentFrame += frames;
}

} // namespace Beam


