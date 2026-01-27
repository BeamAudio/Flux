#include "audio_engine.hpp"
#include <iostream>

namespace Beam {

void AudioEngine::dataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    AudioEngine* pEngine = (AudioEngine*)pDevice->pUserData;
    if (pEngine) {
        pEngine->process((float*)pOutput, (float*)pInput, (int)frameCount);
    }
}

AudioEngine::AudioEngine() : m_sampleRate(44100), m_channels(2) {}

AudioEngine::~AudioEngine() {
    ma_device_uninit(&m_device);
}

bool AudioEngine::init(int sampleRate, int channels) {
    m_sampleRate = sampleRate;
    m_channels = channels;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;
    config.playback.channels = m_channels;
    config.sampleRate        = m_sampleRate;
    config.dataCallback      = dataCallback;
    config.pUserData         = this;

    if (ma_device_init(NULL, &config, &m_device) != MA_SUCCESS) {
        std::cerr << "Failed to initialize miniaudio device." << std::endl;
        return false;
    }

    if (ma_device_start(&m_device) != MA_SUCCESS) {
        std::cerr << "Failed to start miniaudio device." << std::endl;
        return false;
    }

    return true;
}

void AudioEngine::process(float* output, float* input, int frames) {
    if (!m_isPlaying) {
        for (int i = 0; i < frames * m_channels; ++i) output[i] = 0.0f;
        return;
    }

    std::lock_guard<std::mutex> lock(m_nodeMutex);
    
    // Clear output buffer
    for (int i = 0; i < frames * m_channels; ++i) {
        output[i] = 0.0f;
    }

    // Sequential processing for now (Graph traversal later)
    for (auto& node : m_nodes) {
        if (!node->isBypassed()) {
            node->process(output, frames, m_channels);
        }
    }
}

void AudioEngine::addNode(std::shared_ptr<AudioNode> node) {
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    m_nodes.push_back(node);
}

} // namespace Beam
