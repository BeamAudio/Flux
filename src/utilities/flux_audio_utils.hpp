#ifndef FLUX_AUDIO_UTILS_HPP
#define FLUX_AUDIO_UTILS_HPP

#include "engine/audio_buffer.hpp"
#include "render/quad_batcher.hpp"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Beam {

class QuadBatcher;

/**
 * @class AudioUtils
 * @brief Utility functions for audio processing, similar to JUCE's dsp module
 */
class AudioUtils {
public:
    /**
     * @brief Applies a simple linear gain to a buffer
     */
    template<typename T>
    static void applyGain(AudioBuffer<T>& buffer, T gain) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            buffer.applyGain(ch, gain);
        }
    }

    /**
     * @brief Applies a gain ramp over a range of samples
     */
    template<typename T>
    static void applyGainRamp(AudioBuffer<T>& buffer, int channel, int startSample, int numSamples, T startGain, T endGain) {
        if (channel < 0 || channel >= buffer.getNumChannels() || startSample < 0) {
            return;
        }

        int maxSamples = buffer.getNumSamples() - startSample;
        int samplesToDo = (std::min)(numSamples, maxSamples);

        T* data = buffer.getWritePointer(channel) + startSample;
        T increment = (endGain - startGain) / static_cast<T>(samplesToDo);

        for (int i = 0; i < samplesToDo; ++i) {
            T gain = startGain + increment * static_cast<T>(i);
            data[i] *= gain;
        }
    }

    /**
     * @brief Copies audio data from source to destination
     */
    template<typename T>
    static void copyBuffer(AudioBuffer<T>& dest, const AudioBuffer<T>& src, 
                          int destStartSample = 0, int srcStartSample = 0, 
                          int numSamples = -1) {
        int samplesToCopy = (numSamples < 0) ? src.getNumSamples() : numSamples;
        samplesToCopy = (std::min)(samplesToCopy, dest.getNumSamples() - destStartSample);
        samplesToCopy = (std::min)(samplesToCopy, src.getNumSamples() - srcStartSample);

        int channelsToCopy = (std::min)(dest.getNumChannels(), src.getNumChannels());

        for (int ch = 0; ch < channelsToCopy; ++ch) {
            dest.copyFrom(ch, destStartSample, src, ch, srcStartSample, samplesToCopy);
        }
    }

    /**
     * @brief Adds audio data from source to destination
     */
    template<typename T>
    static void addBuffer(AudioBuffer<T>& dest, const AudioBuffer<T>& src, 
                         int destStartSample = 0, int srcStartSample = 0, 
                         int numSamples = -1, T gain = static_cast<T>(1)) {
        int samplesToCopy = (numSamples < 0) ? src.getNumSamples() : numSamples;
        samplesToCopy = (std::min)(samplesToCopy, dest.getNumSamples() - destStartSample);
        samplesToCopy = (std::min)(samplesToCopy, src.getNumSamples() - srcStartSample);

        int channelsToCopy = (std::min)(dest.getNumChannels(), src.getNumChannels());

        for (int ch = 0; ch < channelsToCopy; ++ch) {
            dest.addFrom(ch, destStartSample, src, ch, srcStartSample, samplesToCopy, gain);
        }
    }

    /**
     * @brief Generates a sine wave into the buffer
     */
    template<typename T>
    static void generateSineWave(AudioBuffer<T>& buffer, T frequency, T sampleRate, T amplitude = static_cast<T>(0.5)) {
        int numSamples = buffer.getNumSamples();
        int numChannels = buffer.getNumChannels();

        for (int ch = 0; ch < numChannels; ++ch) {
            T* channelData = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                T phase = static_cast<T>(2.0 * M_PI * frequency * i / sampleRate);
                channelData[i] = amplitude * std::sin(phase);
            }
        }
    }

    /**
     * @brief Clamps a value to a specified range
     */
    template<typename T>
    static T clamp(T value, T min, T max) {
        return (value < min) ? min : ((value > max) ? max : value);
    }

    /**
     * @brief Converts from linear amplitude to decibels
     */
    template<typename T>
    static T linearToDecibels(T linear) {
        if (linear <= static_cast<T>(0)) {
            return static_cast<T>(-100); // Effectively negative infinity
        }
        return static_cast<T>(20.0) * std::log10(linear);
    }

    /**
     * @brief Converts from decibels to linear amplitude
     */
    template<typename T>
    static T decibelsToLinear(T db) {
        return std::pow(static_cast<T>(10.0), db / static_cast<T>(20.0));
    }

    /**
     * @brief Draws text that scrolls horizontally if it exceeds the available width.
     */
    static void drawScrollingText(QuadBatcher& batcher, const std::string& text, float x, float y, float w, float h, float size, float dt, float& timer, float screenHeight) {
        float textWidth = text.length() * size;
        if (textWidth <= w) {
            batcher.drawText(text, x, y, size, 0.9f, 0.9f, 0.9f, 1.0f);
            return;
        }

        timer += dt * 30.0f; // Speed of scroll
        float offset = std::fmod(timer, textWidth + 40.0f);

        batcher.setScissor(x, y, w, h + 10, screenHeight);
        batcher.drawText(text, x - offset, y, size, 0.9f, 0.9f, 0.9f, 1.0f);
        batcher.drawText(text, x - offset + textWidth + 40.0f, y, size, 0.9f, 0.9f, 0.9f, 1.0f);
        batcher.clearScissor();
    }
};

} // namespace Beam

#endif // FLUX_AUDIO_UTILS_HPP





