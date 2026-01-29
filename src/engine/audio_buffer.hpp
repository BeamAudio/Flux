#ifndef AUDIO_BUFFER_HPP
#define AUDIO_BUFFER_HPP

#include <vector>
#include <algorithm>
#include <cstring>

namespace Beam {

/**
 * @class AudioBuffer
 * @brief Container for multi-channel audio data, similar to JUCE's AudioBuffer
 */
template<typename T>
class AudioBuffer {
public:
    /**
     * @brief Constructor - creates an empty buffer
     */
    AudioBuffer();

    /**
     * @brief Constructor - creates a buffer with specified channels and samples
     */
    AudioBuffer(int numChannels, int numSamples);

    /**
     * @brief Destructor
     */
    ~AudioBuffer();

    /**
     * @brief Sets the size of the buffer
     */
    void setSize(int numChannels, int numSamples, bool keepExistingContent = false, bool clearExtraSpace = false, bool avoidReallocating = false);

    /**
     * @brief Returns the number of channels
     */
    int getNumChannels() const { return m_numChannels; }

    /**
     * @brief Returns the number of samples per channel
     */
    int getNumSamples() const { return m_numSamples; }

    /**
     * @brief Returns a pointer to the samples in one of the channels
     */
    T* getWritePointer(int channel) {
        if (channel >= 0 && channel < m_numChannels) {
            return m_data[channel].data();
        }
        return nullptr;
    }

    /**
     * @brief Returns a read-only pointer to the samples in one of the channels
     */
    const T* getReadPointer(int channel) const {
        if (channel >= 0 && channel < m_numChannels) {
            return m_data[channel].data();
        }
        return nullptr;
    }

    /**
     * @brief Clears all the samples in all channels
     */
    void clear();

    /**
     * @brief Clears a specified region in all channels
     */
    void clear(int startSample, int numSamples);

    /**
     * @brief Adds samples from another buffer to this one
     */
    void addFrom(int destChannel, int destStartSample, const AudioBuffer<T>& source, int sourceChannel, int sourceStartSample, int numSamples, T gain = static_cast<T>(1));

    /**
     * @brief Copies data from another buffer
     */
    void copyFrom(int destChannel, int destStartSample, const AudioBuffer<T>& source, int sourceChannel, int sourceStartSample, int numSamples);

    /**
     * @brief Applies gain to a range of samples in a channel
     */
    void applyGain(int channel, int startSample, int numSamples, T gain);

    /**
     * @brief Applies gain to all samples in a channel
     */
    void applyGain(int channel, T gain);

    /**
     * @brief Finds the highest absolute value in a channel
     */
    T getMagnitude(int channel, int startSample, int numSamples) const;

    /**
     * @brief Finds the highest absolute value in the entire buffer
     */
    T getMagnitude() const;

private:
    std::vector<std::vector<T>> m_data;
    int m_numChannels = 0;
    int m_numSamples = 0;
};

// Template implementations
template<typename T>
AudioBuffer<T>::AudioBuffer() {
}

template<typename T>
AudioBuffer<T>::AudioBuffer(int numChannels, int numSamples) {
    setSize(numChannels, numSamples);
}

template<typename T>
AudioBuffer<T>::~AudioBuffer() {
}

template<typename T>
void AudioBuffer<T>::setSize(int numChannels, int numSamples, bool keepExistingContent, bool clearExtraSpace, bool avoidReallocating) {
    if (numChannels == m_numChannels && numSamples == m_numSamples) {
        return;
    }

    if (!keepExistingContent) {
        m_data.clear();
        m_data.resize(numChannels, std::vector<T>(numSamples, static_cast<T>(0)));
    } else {
        // Keep existing content - this is a simplified implementation
        std::vector<std::vector<T>> newData(numChannels, std::vector<T>(numSamples, static_cast<T>(0)));
        
        int channelsToCopy = std::min(numChannels, m_numChannels);
        int samplesToCopy = std::min(numSamples, m_numSamples);
        
        for (int ch = 0; ch < channelsToCopy; ++ch) {
            for (int i = 0; i < samplesToCopy; ++i) {
                newData[ch][i] = m_data[ch][i];
            }
        }
        
        m_data = std::move(newData);
    }
    
    m_numChannels = numChannels;
    m_numSamples = numSamples;
}

template<typename T>
void AudioBuffer<T>::clear() {
    for (auto& channel : m_data) {
        std::fill(channel.begin(), channel.end(), static_cast<T>(0));
    }
}

template<typename T>
void AudioBuffer<T>::clear(int startSample, int numSamples) {
    int endSample = std::min(startSample + numSamples, m_numSamples);
    
    for (auto& channel : m_data) {
        std::fill(channel.begin() + startSample, channel.begin() + endSample, static_cast<T>(0));
    }
}

template<typename T>
void AudioBuffer<T>::addFrom(int destChannel, int destStartSample, const AudioBuffer<T>& source, int sourceChannel, int sourceStartSample, int numSamples, T gain) {
    if (destChannel < 0 || destChannel >= m_numChannels ||
        sourceChannel < 0 || sourceChannel >= source.m_numChannels ||
        destStartSample < 0 || sourceStartSample < 0) {
        return;
    }

    int maxDestSamples = m_numSamples - destStartSample;
    int maxSourceSamples = source.m_numSamples - sourceStartSample;
    int samplesToDo = std::min({numSamples, maxDestSamples, maxSourceSamples});

    T* dest = m_data[destChannel].data() + destStartSample;
    const T* src = source.m_data[sourceChannel].data() + sourceStartSample;

    for (int i = 0; i < samplesToDo; ++i) {
        dest[i] += src[i] * gain;
    }
}

template<typename T>
void AudioBuffer<T>::copyFrom(int destChannel, int destStartSample, const AudioBuffer<T>& source, int sourceChannel, int sourceStartSample, int numSamples) {
    if (destChannel < 0 || destChannel >= m_numChannels ||
        sourceChannel < 0 || sourceChannel >= source.m_numChannels ||
        destStartSample < 0 || sourceStartSample < 0) {
        return;
    }

    int maxDestSamples = m_numSamples - destStartSample;
    int maxSourceSamples = source.m_numSamples - sourceStartSample;
    int samplesToDo = std::min({numSamples, maxDestSamples, maxSourceSamples});

    T* dest = m_data[destChannel].data() + destStartSample;
    const T* src = source.m_data[sourceChannel].data() + sourceStartSample;

    std::copy(src, src + samplesToDo, dest);
}

template<typename T>
void AudioBuffer<T>::applyGain(int channel, int startSample, int numSamples, T gain) {
    if (channel < 0 || channel >= m_numChannels || startSample < 0) {
        return;
    }

    int maxSamples = m_numSamples - startSample;
    int samplesToDo = std::min(numSamples, maxSamples);

    T* data = m_data[channel].data() + startSample;
    for (int i = 0; i < samplesToDo; ++i) {
        data[i] *= gain;
    }
}

template<typename T>
void AudioBuffer<T>::applyGain(int channel, T gain) {
    if (channel < 0 || channel >= m_numChannels) {
        return;
    }

    T* data = m_data[channel].data();
    for (int i = 0; i < m_numSamples; ++i) {
        data[i] *= gain;
    }
}

template<typename T>
T AudioBuffer<T>::getMagnitude(int channel, int startSample, int numSamples) const {
    if (channel < 0 || channel >= m_numChannels || startSample < 0) {
        return static_cast<T>(0);
    }

    int maxSamples = m_numSamples - startSample;
    int samplesToDo = std::min(numSamples, maxSamples);

    const T* data = m_data[channel].data() + startSample;
    T maxVal = static_cast<T>(0);

    for (int i = 0; i < samplesToDo; ++i) {
        T absVal = (data[i] < static_cast<T>(0)) ? -data[i] : data[i];
        if (absVal > maxVal) {
            maxVal = absVal;
        }
    }

    return maxVal;
}

template<typename T>
T AudioBuffer<T>::getMagnitude() const {
    T maxVal = static_cast<T>(0);
    for (int ch = 0; ch < m_numChannels; ++ch) {
        T channelMax = getMagnitude(ch, 0, m_numSamples);
        if (channelMax > maxVal) {
            maxVal = channelMax;
        }
    }
    return maxVal;
}

} // namespace Beam

#endif // AUDIO_BUFFER_HPP


