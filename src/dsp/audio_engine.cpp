#include "audio_engine.hpp"
#include <iostream>

namespace Beam {

AudioEngine::AudioEngine() : m_sampleRate(44100), m_channels(2), m_stream(nullptr) {}

AudioEngine::~AudioEngine() {
    if (m_stream) SDL_DestroyAudioStream(m_stream);
}

bool AudioEngine::init(int sampleRate, int channels) {
    m_sampleRate = sampleRate;
    m_channels = channels;

    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_F32;
    spec.channels = channels;
    spec.freq = sampleRate;

    m_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!m_stream) {
        std::cerr << "SDL_OpenAudioDeviceStream Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_ResumeAudioStreamDevice(m_stream);
    return true;
}

void AudioEngine::process(float* output, int frames) {
    if (!m_isPlaying) {
        for (int i = 0; i < frames * m_channels; ++i) output[i] = 0.0f;
        return;
    }

    std::lock_guard<std::mutex> lock(m_nodeMutex);
    
    // Clear buffer
    for (int i = 0; i < frames * m_channels; ++i) output[i] = 0.0f;

    for (auto& node : m_nodes) {
        if (!node->isBypassed()) {
            node->process(output, frames, m_channels);
        }
    }

    // Push to SDL Stream
    SDL_PutAudioStreamData(m_stream, output, frames * m_channels * sizeof(float));
}

void AudioEngine::addNode(std::shared_ptr<AudioNode> node) {
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    m_nodes.push_back(node);
}

} // namespace Beam