#include "engine/core/audio_device_manager.hpp"
#include <SDL3/SDL.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

namespace Beam {

AudioDeviceManager::AudioDeviceManager() {
}

AudioDeviceManager::~AudioDeviceManager() {
    stopAudio();
}

int AudioDeviceManager::initialise(int numInputChannelsNeeded, int numOutputChannelsNeeded,
                                   const AudioDeviceSetup* preferredSetup,
                                   bool selectDefaultDeviceOnFailure) {
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        std::cerr << "Failed to initialize SDL audio: " << SDL_GetError() << std::endl;
        return -1;
    }

    if (preferredSetup) {
        m_deviceSetup = *preferredSetup;
    } else {
        m_deviceSetup.sampleRate = 44100.0;
        m_deviceSetup.blockSize = 512;
        m_deviceSetup.inputChannels = numInputChannelsNeeded;
        m_deviceSetup.outputChannels = numOutputChannelsNeeded;
    }

    return 0;
}

void AudioDeviceManager::setAudioCallback(std::function<void(const std::map<std::string, float*>&, float**, int, int, int, double)> callback) {
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
    std::cout << "[AudioDeviceManager] Switching Device: " << outputDeviceName << " (" << outputDeviceId << ")" << std::endl;
    bool wasRunning = m_isRunning;
    if (wasRunning) stopAudio();

    m_deviceSetup.outputDeviceName = outputDeviceName;
    m_deviceSetup.outputDeviceId = outputDeviceId;
    m_deviceSetup.inputDeviceName = inputDeviceName;
    m_deviceSetup.inputDeviceId = inputDeviceId;
    m_deviceSetup.sampleRate = sampleRate;
    m_deviceSetup.blockSize = bufferSize;
    
    if (wasRunning) startAudio();
    if (onConfigChanged) onConfigChanged();
    return 0;
}

AudioDeviceSetup AudioDeviceManager::getCurrentDeviceSetup() const {
    return m_deviceSetup;
}

int AudioDeviceManager::startAudio() {
    if (m_isRunning) return 0;

    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_F32;
    spec.channels = m_deviceSetup.outputChannels;
    spec.freq = (int)m_deviceSetup.sampleRate;

    SDL_AudioDeviceID outId = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
    if (!m_deviceSetup.outputDeviceId.empty()) {
        try { 
            outId = (SDL_AudioDeviceID)std::stoul(m_deviceSetup.outputDeviceId); 
        } catch(...) {
            std::cerr << "[AudioDeviceManager] Error parsing output ID: " << m_deviceSetup.outputDeviceId << std::endl;
        }
    }

    std::cout << "[AudioDeviceManager] Opening Output Stream on Device ID: " << outId << " (" << (outId == SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK ? "Default" : "Specific") << ")" << std::endl;
    m_outputStream = SDL_OpenAudioDeviceStream(outId, &spec, sdlAudioCallback, this);
    if (!m_outputStream) {
        std::cerr << "[AudioDeviceManager] SDL_OpenAudioDeviceStream failed: " << SDL_GetError() << std::endl;
        return -1;
    }
    
    SDL_AudioSpec actual;
    if (SDL_GetAudioStreamFormat(m_outputStream, &actual, NULL)) {
        std::cout << "[AudioDeviceManager] Stream opened. Hardware Rate: " << actual.freq << "Hz, Channels: " << (int)actual.channels << std::endl;
    }

    SDL_ResumeAudioStreamDevice(m_outputStream);
    m_isRunning = true;
    return 0;
}

void AudioDeviceManager::stopAudio() {
    if (!m_isRunning) return;
    std::cout << "[AudioDeviceManager] Stopping Audio." << std::endl;
    if (m_outputStream) {
        SDL_DestroyAudioStream(m_outputStream);
        m_outputStream = nullptr;
    }
    for (auto& [id, stream] : m_inputStreams) {
        SDL_DestroyAudioStream(stream);
    }
    m_inputStreams.clear();
    m_isRunning = false;
}

SDL_AudioStream* AudioDeviceManager::openInputStream(const std::string& deviceId) {
    if (m_inputStreams.count(deviceId)) return m_inputStreams[deviceId];

    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2; // Default to stereo for now
    spec.freq = (int)m_deviceSetup.sampleRate;

    SDL_AudioDeviceID inId = SDL_AUDIO_DEVICE_DEFAULT_RECORDING;
    if (!deviceId.empty()) {
        try { inId = (SDL_AudioDeviceID)std::stoul(deviceId); } catch(...) { return nullptr; }
    }
    
    std::cout << "[AudioDeviceManager] Opening Input Stream on Device ID: " << inId << " (" << (deviceId.empty() ? "Default" : deviceId) << ")" << std::endl;
    SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(inId, &spec, NULL, NULL);
    if (stream) {
        SDL_ResumeAudioStreamDevice(stream);
        m_inputStreams[deviceId] = stream;
        return stream;
    } else {
        std::cerr << "[AudioDeviceManager] Failed to open input stream: " << SDL_GetError() << std::endl;
    }
    return nullptr;
}

void SDLCALL AudioDeviceManager::sdlAudioCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount) {
    AudioDeviceManager* manager = static_cast<AudioDeviceManager*>(userdata);
    if (!manager || !manager->m_audioCallback) return;

    if (additional_amount <= 0) return;

    int maxBlockSize = manager->m_deviceSetup.blockSize;
    int channels = manager->m_deviceSetup.outputChannels;
    int frameSize = channels * sizeof(float);
    int totalFramesNeeded = additional_amount / frameSize;

    std::vector<float> outBuf(maxBlockSize * channels);
    static std::map<std::string, std::vector<float>> inBuffers;

    while (totalFramesNeeded > 0) {
        int frames = (std::min)(totalFramesNeeded, maxBlockSize);
        
        std::map<std::string, float*> inputMap;
        for (auto& [id, inStream] : manager->m_inputStreams) {
            auto& buf = inBuffers[id];
            buf.resize(frames * 2); 
            
            int inputFrameSize = 2 * sizeof(float);
            int availBytes = SDL_GetAudioStreamAvailable(inStream);
            int bytesToRead = frames * inputFrameSize;
            
            if (availBytes >= bytesToRead) {
                SDL_GetAudioStreamData(inStream, buf.data(), bytesToRead);
            } else {
                int read = SDL_GetAudioStreamData(inStream, buf.data(), availBytes);
                if (read < bytesToRead && read > 0) {
                    std::memset((char*)buf.data() + read, 0, bytesToRead - read);
                } else if (read <= 0) {
                    std::memset(buf.data(), 0, bytesToRead);
                }
            }
            inputMap[id] = buf.data();
        }

        float* outChannels[2] = { outBuf.data(), nullptr };
        manager->m_audioCallback(inputMap, outChannels, frames, 2, channels, manager->m_deviceSetup.sampleRate);

        SDL_PutAudioStreamData(stream, outBuf.data(), frames * channels * sizeof(float));
        totalFramesNeeded -= frames;
    }
}

bool AudioDeviceManager::isAudioRunning() const { return m_isRunning; }
double AudioDeviceManager::getCurrentSampleRate() const { return m_deviceSetup.sampleRate; }
int AudioDeviceManager::getCurrentBufferSizeSamples() const { return m_deviceSetup.blockSize; }

} // namespace Beam
