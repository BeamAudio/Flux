#ifndef FLUX_AUDIO_UTILS_HPP
#define FLUX_AUDIO_UTILS_HPP

#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <sstream>

#include <xmmintrin.h>
#include <emmintrin.h>

namespace Beam {

/**
 * @class SIMD
 * @brief High-performance audio arithmetic using SSE.
 */
class SIMD {
public:
    /**
     * @brief Adds source buffer into destination buffer: dst = dst + src
     */
    static void add(float* dst, const float* src, int count) {
        int i = 0;
        // Process blocks of 4 floats
        for (; i <= count - 4; i += 4) {
            __m128 vsrc = _mm_loadu_ps(src + i);
            __m128 vdst = _mm_loadu_ps(dst + i);
            _mm_storeu_ps(dst + i, _mm_add_ps(vdst, vsrc));
        }
        // Remaining elements
        for (; i < count; ++i) dst[i] += src[i];
    }

    static void copy(float* dst, const float* src, int count) {
        memcpy(dst, src, count * sizeof(float));
    }

    static void set(float* dst, float val, int count) {
        int i = 0;
        __m128 vval = _mm_set1_ps(val);
        for (; i <= count - 4; i += 4) {
            _mm_storeu_ps(dst + i, vval);
        }
        for (; i < count; ++i) dst[i] = val;
    }

    /**
     * @brief Adds source to dest with a scalar gain.
     * dst[i] += src[i] * gain
     */
    static void add_with_gain(const float* src, float* dst, float gain, int count) {
        int i = 0;
        __m128 vGain = _mm_set1_ps(gain);
        for (; i <= count - 4; i += 4) {
            __m128 vsrc = _mm_loadu_ps(src + i);
            __m128 vdst = _mm_loadu_ps(dst + i);
            __m128 vScaled = _mm_mul_ps(vsrc, vGain);
            vdst = _mm_add_ps(vdst, vScaled);
            _mm_storeu_ps(dst + i, vdst);
        }
        for (; i < count; ++i) {
            dst[i] += src[i] * gain;
        }
    }

    /**
     * @brief Adds interleaved stereo source to dest with gain and pan.
     */
    static void add_with_gain_pan(const float* src, float* dst, float gain, float panL, float panR, int frames) {
        int i = 0;
        __m128 vG = _mm_set1_ps(gain);
        __m128 vP = _mm_set_ps(panR, panL, panR, panL);
        __m128 vGP = _mm_mul_ps(vG, vP);

        for (; i <= frames - 2; i += 2) {
            __m128 vsrc = _mm_loadu_ps(src + i * 2);
            __m128 vdst = _mm_loadu_ps(dst + i * 2);
            __m128 vScaled = _mm_mul_ps(vsrc, vGP);
            vdst = _mm_add_ps(vdst, vScaled);
            _mm_storeu_ps(dst + i * 2, vdst);
        }
        for (; i < frames; ++i) {
            dst[i * 2] += src[i * 2] * gain * panL;
            dst[i * 2 + 1] += src[i * 2 + 1] * gain * panR;
        }
    }

    /**
     * @brief Calculates the sum of squared differences for YIN algorithm.
     * sum((buffer[j] - buffer[j+tau])^2)
     */
    static float yin_difference(const float* buffer, int tau, int N) {
        float diff = 0.0f;
        __m128 vDiff = _mm_setzero_ps();
        int j = 0;
        
        for (; j <= N - 4; j += 4) {
            __m128 v1 = _mm_loadu_ps(&buffer[j]);
            __m128 v2 = _mm_loadu_ps(&buffer[j + tau]);
            __m128 vD = _mm_sub_ps(v1, v2);
            __m128 vSq = _mm_mul_ps(vD, vD);
            vDiff = _mm_add_ps(vDiff, vSq);
        }
        
        // Horizontal sum
        __m128 vShuf = _mm_shuffle_ps(vDiff, vDiff, _MM_SHUFFLE(2, 3, 0, 1));
        vDiff = _mm_add_ps(vDiff, vShuf);
        vShuf = _mm_movehl_ps(vShuf, vDiff);
        vDiff = _mm_add_ss(vDiff, vShuf);
        diff = _mm_cvtss_f32(vDiff);

        // Scalar remainder
        for (; j < N; ++j) {
            float d = buffer[j] - buffer[j + tau];
            diff += d * d;
        }
        return diff;
    }
};

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
     * Note: This matches QuadBatcher::drawText which uses 'size' per character.
     */
    static float calculateTextWidth(const std::string& text, float size) {
        return (float)text.length() * size; 
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
