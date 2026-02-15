#ifndef FLUX_AUDIO_UTILS_HPP
#define FLUX_AUDIO_UTILS_HPP

#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <sstream>

#include <immintrin.h>

namespace Beam {

/**
 * @class SIMD
 * @brief High-performance audio arithmetic using SSE and AVX2.
 */
class SIMD {
public:
    /**
     * @brief Adds source buffer into destination buffer: dst = dst + src
     */
    static void add(float* dst, const float* src, int count) {
        int i = 0;
#ifdef __AVX2__
        // Process blocks of 8 floats (AVX2)
        for (; i <= count - 8; i += 8) {
            __m256 vsrc = _mm256_loadu_ps(src + i);
            __m256 vdst = _mm256_loadu_ps(dst + i);
            _mm256_storeu_ps(dst + i, _mm256_add_ps(vdst, vsrc));
        }
#endif
        // Process blocks of 4 floats (SSE)
        for (; i <= count - 4; i += 4) {
            __m128 vsrc = _mm_loadu_ps(src + i);
            __m128 vdst = _mm_loadu_ps(dst + i);
            _mm_storeu_ps(dst + i, _mm_add_ps(vdst, vsrc));
        }
        for (; i < count; ++i) dst[i] += src[i];
    }

    static void copy(float* dst, const float* src, int count) {
        if (count <= 0) return;
        memcpy(dst, src, count * sizeof(float));
    }

    static void set(float* dst, float val, int count) {
        int i = 0;
#ifdef __AVX2__
        __m256 vval256 = _mm256_set1_ps(val);
        for (; i <= count - 8; i += 8) {
            _mm256_storeu_ps(dst + i, vval256);
        }
#endif
        __m128 vval128 = _mm_set1_ps(val);
        for (; i <= count - 4; i += 4) {
            _mm_storeu_ps(dst + i, vval128);
        }
        for (; i < count; ++i) dst[i] = val;
    }

    /**
     * @brief Adds source to dest with a scalar gain.
     */
    static void add_with_gain(const float* src, float* dst, float gain, int count) {
        int i = 0;
#ifdef __AVX2__
        __m256 vGain256 = _mm256_set1_ps(gain);
        for (; i <= count - 8; i += 8) {
            __m256 vsrc = _mm256_loadu_ps(src + i);
            __m256 vdst = _mm256_loadu_ps(dst + i);
            _mm256_storeu_ps(dst + i, _mm256_add_ps(vdst, _mm256_mul_ps(vsrc, vGain256)));
        }
#endif
        __m128 vGain128 = _mm_set1_ps(gain);
        for (; i <= count - 4; i += 4) {
            __m128 vsrc = _mm_loadu_ps(src + i);
            __m128 vdst = _mm_loadu_ps(dst + i);
            _mm_storeu_ps(dst + i, _mm_add_ps(vdst, _mm_mul_ps(vsrc, vGain128)));
        }
        for (; i < count; ++i) dst[i] += src[i] * gain;
    }

    /**
     * @brief High-quality stereo panning with stereo image preservation.
     * 
     * Uses "Balance + Crossfeed" approach:
     * - At center (pan=0.5): Full stereo separation, unity gain
     * - Panning left: Right channel gradually crossfades into left
     * - Panning right: Left channel gradually crossfades into right
     * - Constant power law applied for smooth transitions
     */
    static void add_with_gain_pan(const float* src, float* dst, float gain, float panL, float panR, int frames) {
        // panL/panR are already cos/sin of (pan * pi/2)
        // pan=0.5 gives panL=panR~=0.707 (center)
        // pan=0 gives panL=1, panR=0 (hard left)
        // pan=1 gives panL=0, panR=1 (hard right)
        
        for (int i = 0; i < frames; ++i) {
            float srcL = src[i * 2] * gain;
            float srcR = src[i * 2 + 1] * gain;
            
            // Calculate balance from pan coefficients (0=left, 0.5=center, 1=right)
            float balance = panR / (panL + panR + 1e-6f);
            
            // Crossfeed amount: 0 at center, increases toward extremes
            float crossfeed = std::abs(balance - 0.5f) * 2.0f;
            
            // Mix channels based on pan direction
            float mixL = srcL + srcR * crossfeed * (1.0f - balance);
            float mixR = srcR + srcL * crossfeed * balance;
            
            // Apply pan coefficients
            dst[i * 2]     += mixL * panL;
            dst[i * 2 + 1] += mixR * panR;
        }
    }

    /**
     * @brief Optimized Direct Form II Transposed Biquad Filter.
     * Applies the filter in-place to the buffer.
     */
    static void process_biquad(float* buffer, int count, float b0, float b1, float b2, float a1, float a2, float& z1, float& z2) {
        for (int i = 0; i < count; ++i) {
            float in = buffer[i];
            float out = in * b0 + z1;
            z1 = in * b1 - out * a1 + z2;
            z2 = in * b2 - out * a2;
            buffer[i] = out;
        }
    }

    /**
     * @brief Calculates the sum of squared differences for YIN algorithm.
     */
    static float yin_difference(const float* buffer, int tau, int N) {
        float diff = 0.0f;
        int j = 0;
        __m128 vDiff = _mm_setzero_ps();
        
        for (; j <= N - 4; j += 4) {
            __m128 v1 = _mm_loadu_ps(&buffer[j]);
            __m128 v2 = _mm_loadu_ps(&buffer[j + tau]);
            __m128 vD = _mm_sub_ps(v1, v2);
            vDiff = _mm_add_ps(vDiff, _mm_mul_ps(vD, vD));
        }
        
        __m128 vShuf = _mm_shuffle_ps(vDiff, vDiff, _MM_SHUFFLE(2, 3, 0, 1));
        vDiff = _mm_add_ps(vDiff, vShuf);
        vShuf = _mm_movehl_ps(vShuf, vDiff);
        vDiff = _mm_add_ss(vDiff, vShuf);
        diff = _mm_cvtss_f32(vDiff);

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
        float total = 0;
        for (char c : text) {
            char up = std::toupper((unsigned char)c);
            if (up == 'I' || up == '.' || up == ',' || up == ':' || up == ';') total += size * 0.55f;
            else if (up == ' ') total += size * 0.8f;
            else if (up == 'L' || up == 'F' || up == 'T' || up == '1') total += size * 0.75f;
            else if (up == 'M' || up == 'W') total += size * 1.0f;
            else if (up == 'D' || up == 'G' || up == 'O' || up == 'Q' || up == '@') total += size * 0.95f;
            else total += size * 0.9f;
        }
        return total;
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
