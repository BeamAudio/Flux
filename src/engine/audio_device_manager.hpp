#ifndef AUDIO_DEVICE_MANAGER_HPP
#define AUDIO_DEVICE_MANAGER_HPP

#include <string>
#include <vector>
#include <functional>

namespace Beam {

struct AudioDeviceSetup {
    std::string outputDeviceName;
    std::string outputDeviceId;
    std::string inputDeviceName;
    std::string inputDeviceId;
    double sampleRate = 44100.0;
    int blockSize = 512;
    int inputChannels = 2;
    int outputChannels = 2;
};

struct AudioDeviceInfo {
    std::string name;
    std::string deviceId; // Backend-specific identifier
    int maxInputChannels = 0;
    int maxOutputChannels = 0;
    std::vector<double> sampleRates;
    std::vector<int> bufferSizes;
};

/**
 * @class AudioDeviceManager
 * @brief Manages audio devices, similar to JUCE's AudioDeviceManager
 */
class AudioDeviceManager {
public:
    AudioDeviceManager();
    ~AudioDeviceManager();

    /**
     * @brief Initializes the audio device manager
     */
    int initialise(int numInputChannelsNeeded, int numOutputChannelsNeeded,
                   const AudioDeviceSetup* preferredSetup = nullptr,
                   bool selectDefaultDeviceOnFailure = true);

    /**
     * @brief Sets the audio callback
     */
    void setAudioCallback(std::function<void(float**, float**, int, int, int, double)> callback);

    /**
     * @brief Gets a list of available output devices
     */
    std::vector<AudioDeviceInfo> getAvailableOutputDevices() const;

    /**
     * @brief Gets a list of available input devices
     */
    std::vector<AudioDeviceInfo> getAvailableInputDevices() const;

    /**
     * @brief Sets the current audio device
     */
    int setCurrentAudioDevice(const std::string& outputDeviceName,
                              const std::string& outputDeviceId,
                              const std::string& inputDeviceName,
                              const std::string& inputDeviceId,
                              double sampleRate,
                              int bufferSize);

    /**
     * @brief Gets the current device setup
     */
    AudioDeviceSetup getCurrentDeviceSetup() const;

    /**
     * @brief Starts the audio stream
     */
    int startAudio();

    /**
     * @brief Stops the audio stream
     */
    void stopAudio();

    /**
     * @brief Checks if audio is currently running
     */
    bool isAudioRunning() const;

    /**
     * @brief Gets the current sample rate
     */
    double getCurrentSampleRate() const;

    /**
     * @brief Gets the current block size
     */
    int getCurrentBufferSizeSamples() const;

private:
    AudioDeviceSetup m_deviceSetup;
    bool m_isRunning = false;
    std::function<void(float**, float**, int, int, int, double)> m_audioCallback;
};

} // namespace Beam

#endif // AUDIO_DEVICE_MANAGER_HPP





