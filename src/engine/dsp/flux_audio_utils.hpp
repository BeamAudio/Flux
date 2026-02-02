#ifndef FLUX_AUDIO_UTILS_HPP
#define FLUX_AUDIO_UTILS_HPP

#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <sstream>

namespace Beam {

/**
 * @class AudioUtils
 * @brief Static utility functions for audio processing and UI helper calculations.
 */
class AudioUtils {
public:
    /**
     * @brief Normalizes a value from [min, max] to [0.0, 1.0].
     */
    static float normalize(float value, float min, float max) {
        if (std::abs(max - min) < 1e-6f) return 0.0f;
        return std::clamp((value - min) / (max - min), 0.0f, 1.0f);
    }

    /**
     * @brief Denormalizes a value from [0.0, 1.0] to [min, max].
     */
    static float denormalize(float normalizedValue, float min, float max) {
        return min + normalizedValue * (max - min);
    }

    /**
     * @brief Converts decibels to a linear gain factor.
     */
    static float dbToGain(float db) {
        return std::pow(10.0f, db * 0.05f);
    }

    /**
     * @brief Converts a linear gain factor to decibels.
     */
    static float gainToDb(float gain) {
        if (gain <= 0.0f) return -100.0f;
        return 20.0f * std::log10(gain);
    }

    /**
     * @brief Frequency to Normalized (logarithmic mapping for filters).
     */
    static float freqToNorm(float freq, float minFreq, float maxFreq) {
        if (freq <= 0.0f || minFreq <= 0.0f || maxFreq <= minFreq) return 0.0f;
        return std::log2(freq / minFreq) / std::log2(maxFreq / minFreq);
    }

    /**
     * @brief Normalized to Frequency (logarithmic mapping).
     */
    static float normToFreq(float norm, float minFreq, float maxFreq) {
        return minFreq * std::pow(2.0f, norm * std::log2(maxFreq / minFreq));
    }

    /**
     * @brief Converts a frequency to a wavelength in samples.
     */
    static float freqToWavelength(float freq, float sampleRate) {
        if (freq <= 0.0f) return 0.0f;
        return sampleRate / freq;
    }

    /**
     * @brief Calculates a fast approximation of the signal's RMS level.
     */
    static float calculateRMS(const float* buffer, int numSamples) {
        if (numSamples <= 0) return 0.0f;
        float sum = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            sum += buffer[i] * buffer[i];
        }
        return std::sqrt(sum / (float)numSamples);
    }

    /**
     * @brief Applies a simple one-pole smoothing filter.
     * @param target The desired value.
     * @param current The current value.
     * @param coeff Smoothing coefficient (0.0 to 1.0, lower = smoother).
     */
    static float smooth(float target, float current, float coeff) {
        return current + coeff * (target - current);
    }

    /**
     * @brief Clamps a frequency to within the Nyquist limit.
     */
    static float clampFrequency(float freq, float sampleRate) {
        return std::clamp(freq, 20.0f, sampleRate * 0.49f);
    }

    /**
     * @brief Calculates the width of a string in pixels given a font size.
     * Note: This assumes a fixed-width bitmapped font where each char is 'size' wide.
     */
    static float calculateTextWidth(const std::string& text, float size) {
        return (float)text.length() * size * 0.9f; 
    }

    /**
     * @brief Calculates how many lines are needed to display text within maxWidth.
     */
    static int calculateLineCount(const std::string& text, float fontSize, float maxWidth) {
        if (maxWidth <= 0) return 1;
        auto lines = wrapText(text, fontSize, maxWidth);
        return (int)lines.size();
    }

    /**
     * @brief Wraps text into multiple lines to fit within maxWidth using proper word boundaries.
     */
    static std::vector<std::string> wrapText(const std::string& text, float fontSize, float maxWidth) {
        std::vector<std::string> lines;
        if (text.empty()) return lines;
        
        // If maxWidth is too small to fit even one char, just return as is (to avoid infinite loops)
        if (maxWidth < fontSize) {
            lines.push_back(text);
            return lines;
        }

        std::stringstream ss(text);
        std::string word;
        std::string currentLine;
        float currentLineWidth = 0;

        while (ss >> word) {
            float wordWidth = calculateTextWidth(word, fontSize);
            float spaceWidth = calculateTextWidth(" ", fontSize);

            if (currentLine.empty()) {
                currentLine = word;
                currentLineWidth = wordWidth;
            } else {
                if (currentLineWidth + spaceWidth + wordWidth <= maxWidth) {
                    currentLine += " " + word;
                    currentLineWidth += spaceWidth + wordWidth;
                } else {
                    lines.push_back(currentLine);
                    currentLine = word;
                    currentLineWidth = wordWidth;
                }
            }
        }
        
        if (!currentLine.empty()) {
            lines.push_back(currentLine);
        }

        return lines;
    }

    /**
     * @brief Simple Pan Law calculation (constant power).
     * @param pan Pan position from -1.0 (left) to 1.0 (right).
     * @param leftGain Output gain for left channel.
     * @param rightGain Output gain for right channel.
     */
    static void panLaw(float pan, float& leftGain, float& rightGain) {
        float p = (pan + 1.0f) * 0.5f; // 0.0 to 1.0
        leftGain = std::cos(p * 1.570796f);
        rightGain = std::sin(p * 1.570796f);
    }
};

} // namespace Beam

#endif // FLUX_AUDIO_UTILS_HPP
