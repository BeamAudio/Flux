#include "audio_engine.hpp"
#include "flux_track_node.hpp"
#include <iostream>

namespace Beam {

AudioEngine::AudioEngine() : m_sampleRate(44100), m_channels(2), m_stream(nullptr) {}

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
    std::lock_guard<std::mutex> lock(m_engineMutex);
    m_graph = graph;
    if (m_graph) {
        m_masterNodeId = m_graph->addNode(m_masterNode);
    }
}

void AudioEngine::rewind() {
    std::lock_guard<std::mutex> lock(m_engineMutex);
    if (!m_graph) return;
    
    // Iterate through all nodes and seek to 0 if they are FluxTrackNodes
    // This is a bit of a hacky way since we don't have a generic "seekable" interface yet
    // but works for now.
        for (auto const& [id, node] : m_graph->getNodes()) {
            auto trackNode = std::dynamic_pointer_cast<FluxTrackNode>(node);
            if (trackNode) {
                trackNode->seek(0);
            }
        }
    }
    
    void AudioEngine::process(float* output, int frames) {
        if (!m_isPlaying || !m_stream || !m_graph) {
            for (int i = 0; i < frames * m_channels; ++i) output[i] = 0.0f;
            return;
        }
    
        std::lock_guard<std::mutex> lock(m_engineMutex);
        
        m_graph->process(frames);
    
        // Copy from MasterNode input to output
        float* masterIn = m_masterNode->getInputBuffer(0);
        for (int i = 0; i < frames * m_channels; ++i) {
            output[i] = masterIn[i];
        }
    
        // Push to SDL Stream
        SDL_PutAudioStreamData(m_stream, output, frames * m_channels * sizeof(float));
    }
    
    } // namespace Beam
    