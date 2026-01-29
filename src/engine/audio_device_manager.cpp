#include "audio_device_manager.hpp"
#include <SDL3/SDL.h>
#include <iostream>

namespace Beam {

AudioDeviceManager::AudioDeviceManager() {
}

AudioDeviceManager::~AudioDeviceManager() {
    stopAudio();
}

int AudioDeviceManager::initialise(int numInputChannelsNeeded, int numOutputChannelsNeeded,
                                   const AudioDeviceSetup* preferredSetup,
                                   bool selectDefaultDeviceOnFailure) {
    // Initialize SDL audio
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        std::cerr << "Failed to initialize SDL audio: " << SDL_GetError() << std::endl;
        return -1;
    }

    if (preferredSetup) {
        m_deviceSetup = *preferredSetup;
    } else {
        // Set defaults
        m_deviceSetup.sampleRate = 44100.0;
        m_deviceSetup.blockSize = 512;
        m_deviceSetup.inputChannels = numInputChannelsNeeded;
        m_deviceSetup.outputChannels = numOutputChannelsNeeded;
    }

    return 0;
}

void AudioDeviceManager::setAudioCallback(std::function<void(float**, float**, int, int, int, double)> callback) {
    m_audioCallback = callback;
}

std::vector<AudioDeviceInfo> AudioDeviceManager::getAvailableOutputDevices() const {
    std::vector<AudioDeviceInfo> devices;
    
    int count = 0;
    SDL_AudioDeviceID* sdlDevices = SDL_GetAudioPlaybackDevices(&count);
    
    if (sdlDevices) {
        for (int i = 0; i < count; ++i) {
            AudioDeviceInfo info;
            const char* name = SDL_GetAudioDeviceName(sdlDevices[i]);
            info.name = name ? name : "Unknown Device";
            info.deviceId = std::to_string(sdlDevices[i]);
            
            SDL_AudioSpec spec;
            if (SDL_GetAudioDeviceFormat(sdlDevices[i], &spec, NULL)) {
                info.maxOutputChannels = spec.channels;
                info.sampleRates = {(double)spec.freq};
            }
            
            info.bufferSizes = {128, 256, 512, 1024, 2048};
            devices.push_back(info);
        }
        SDL_free(sdlDevices);
    }
    
    return devices;
}

std::vector<AudioDeviceInfo> AudioDeviceManager::getAvailableInputDevices() const {
    std::vector<AudioDeviceInfo> devices;
    
    int count = 0;
    SDL_AudioDeviceID* sdlDevices = SDL_GetAudioRecordingDevices(&count);
    
    if (sdlDevices) {
        for (int i = 0; i < count; ++i) {
            AudioDeviceInfo info;
            const char* name = SDL_GetAudioDeviceName(sdlDevices[i]);
            info.name = name ? name : "Unknown Device";
            info.deviceId = std::to_string(sdlDevices[i]);
            
            SDL_AudioSpec spec;
            if (SDL_GetAudioDeviceFormat(sdlDevices[i], &spec, NULL)) {
                info.maxInputChannels = spec.channels;
                info.sampleRates = {(double)spec.freq};
            }
            
            info.bufferSizes = {128, 256, 512, 1024, 2048};
            devices.push_back(info);
        }
        SDL_free(sdlDevices);
    }
    
    return devices;
}

int AudioDeviceManager::setCurrentAudioDevice(const std::string& outputDeviceName,
                                              const std::string& outputDeviceId,
                                              const std::string& inputDeviceName,
                                              const std::string& inputDeviceId,
                                              double sampleRate,
                                              int bufferSize) {
    m_deviceSetup.outputDeviceName = outputDeviceName;
    m_deviceSetup.outputDeviceId = outputDeviceId;
    m_deviceSetup.inputDeviceName = inputDeviceName;
    m_deviceSetup.inputDeviceId = inputDeviceId;
    m_deviceSetup.sampleRate = sampleRate;
    m_deviceSetup.blockSize = bufferSize;
    
    return 0;
}

AudioDeviceSetup AudioDeviceManager::getCurrentDeviceSetup() const {
    return m_deviceSetup;
}

int AudioDeviceManager::startAudio() {
    if (m_isRunning) {
        return 0; // Already running
    }

    // In a real implementation, this would start the audio stream
    // For now, we'll just set the flag
    m_isRunning = true;
    
    std::cout << "Audio started with sample rate: " << m_deviceSetup.sampleRate 
              << ", block size: " << m_deviceSetup.blockSize << std::endl;
    
    return 0;
}

void AudioDeviceManager::stopAudio() {
    if (!m_isRunning) {
        return;
    }
    
    m_isRunning = false;
    std::cout << "Audio stopped" << std::endl;
}

bool AudioDeviceManager::isAudioRunning() const {
    return m_isRunning;
}

double AudioDeviceManager::getCurrentSampleRate() const {
    return m_deviceSetup.sampleRate;
}

int AudioDeviceManager::getCurrentBufferSizeSamples() const {
    return m_deviceSetup.blockSize;
}

} // namespace Beam





