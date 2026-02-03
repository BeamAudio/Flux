#ifndef PITCH_FX_NODES_HPP
#define PITCH_FX_NODES_HPP

#include "engine/plugins/flux_plugin.hpp"
#include "engine/dsp/flux_audio_utils.hpp"
#include <vector>
#include <cmath>
#include <algorithm>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <iostream>

namespace Beam {

/**
 * @class AutoTuneProcessor
 * @brief Simple real-time pitch corrector with background YIN analysis.
 */
class AutoTuneProcessor : public FluxPluginProcessor {
public:
    AutoTuneProcessor(float sr) : FluxPluginProcessor(sr), m_sampleRate(sr) {
        m_delayBuffer.resize((size_t)(sr * 0.2f), 0.0f);
        m_analysisBufferFront.resize(2048, 0.0f);
        m_analysisBufferBack.resize(2048, 0.0f);
        m_yinBuffer.resize(1024, 0.0f);
        
        m_workerRunning = true;
        m_workerThread = std::thread([this]() { workerLoop(); });
    }

    ~AutoTuneProcessor() {
        m_workerRunning = false;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_hasWork = true; // Wake up to exit
        }
        m_cv.notify_all();
        if (m_workerThread.joinable()) m_workerThread.join();
    }

    void processBlock(const float* in, float* out, int total) override {
        float userShift = getParam(0); 
        float retuneSpeed = getParam(1); 
        
        for (int i = 0; i < total; ++i) {
            float sample = in[i];
            m_delayBuffer[m_writePos] = sample;
            
            // 1. Fill Analysis Buffer (Snapshotting)
            // Write to Front buffer
            m_analysisBufferFront[m_analysisWritePos] = sample;
            m_analysisWritePos = (m_analysisWritePos + 1) % m_analysisBufferFront.size();

            // Notify worker thread every 512 samples
            if (++m_analysisCounter >= 512) {
                m_analysisCounter = 0;
                
                // Swap buffers: Back becomes Front (for writing), Front becomes Back (for reading)
                // We only swap if the worker is done, otherwise we skip this analysis frame (drop frame)
                // to avoid tearing or blocking.
                if (!m_hasWork) {
                    std::swap(m_analysisBufferFront, m_analysisBufferBack);
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        m_hasWork = true;
                    }
                    m_cv.notify_one();
                }
            }

            // 2. Determine Pitch Shift
            float currentFreq = m_detectedFreq.load(std::memory_order_relaxed);
            float finalShift = userShift;

            if (m_isVocalActive.load(std::memory_order_relaxed) && retuneSpeed > 0.01f && currentFreq > 40.0f) {
                float semitones = 12.0f * std::log2(currentFreq / 440.0f);
                float snapped = std::round(semitones);
                float targetFreq = 440.0f * std::pow(2.0f, snapped / 12.0f);
                
                float autoShift = targetFreq / currentFreq;
                finalShift = userShift * (1.0f - retuneSpeed) + (autoShift * userShift) * retuneSpeed;
            }

            // 3. Dual-Tap Pitch Shifter
            float phase1 = m_phase;
            float phase2 = std::fmod(m_phase + 0.5f, 1.0f);
            float weight = std::abs(phase1 - 0.5f) * 2.0f;
            
            float out1 = readDelay(phase1 * m_maxDelay);
            float out2 = readDelay(phase2 * m_maxDelay);
            out[i] = out1 * (1.0f - weight) + out2 * weight;

            if (++m_writePos >= m_delayBuffer.size()) m_writePos = 0;
            m_phase = std::fmod(m_phase + (1.0f - finalShift) / m_maxDelay, 1.0f);
            if (m_phase < 0) m_phase += 1.0f;
        }
    }

private:
    void workerLoop() {
        std::vector<float> snapshot(2048);
        while (m_workerRunning) {
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [this] { return m_hasWork || !m_workerRunning; });
                m_hasWork = false;
            }
            if (!m_workerRunning) break;

            // Process 'Back' buffer which is stable now
            estimatePitchAndVAD(m_analysisBufferBack);
        }
    }

    void estimatePitchAndVAD(const std::vector<float>& buffer) {
        const int N = 600; // Reduced from 1024 to 600 (covers down to ~73Hz) for performance
        const float threshold = 0.15f;
        
        float energy = 0.0f;
        // Simple energy check
        for (size_t i = 0; i < N; ++i) energy += buffer[i] * buffer[i];
        energy = std::sqrt(energy / (float)N);
        
        // Difference function (SIMD Optimized)
        for (int tau = 0; tau < N; ++tau) {
            m_yinBuffer[tau] = SIMD::yin_difference(buffer.data(), tau, N);
        }

        m_yinBuffer[0] = 1.0f;
        float runningSum = 0.0f;
        for (int tau = 1; tau < N; ++tau) {
            runningSum += m_yinBuffer[tau];
            m_yinBuffer[tau] *= (float)tau / (runningSum + 1e-6f);
        }

        int tauFound = -1;
        float minVal = 1.0f;
        for (int tau = 20; tau < N; ++tau) {
            if (m_yinBuffer[tau] < threshold) {
                tauFound = tau;
                break;
            }
            if (m_yinBuffer[tau] < minVal) minVal = m_yinBuffer[tau];
        }

        float clarity = 1.0f - minVal;
        m_isVocalActive.store(energy > 0.01f && clarity > 0.6f, std::memory_order_relaxed);

        if (tauFound != -1) {
            float freq = m_sampleRate / (float)tauFound; 
            if (freq > 40.0f && freq < 2000.0f) {
                m_detectedFreq.store(freq, std::memory_order_relaxed);
            }
        }
    }

    float readDelay(float samples) {
        float pos = (float)m_writePos - samples;
        while (pos < 0) pos += (float)m_delayBuffer.size();
        int i1 = (int)pos % (int)m_delayBuffer.size();
        int i2 = (i1 + 1) % (int)m_delayBuffer.size();
        float frac = pos - (float)i1;
        return m_delayBuffer[i1] * (1.0f - frac) + m_delayBuffer[i2] * frac;
    }

    float m_sampleRate;
    std::vector<float> m_delayBuffer;
    
    // Double buffered analysis to prevent race conditions
    std::vector<float> m_analysisBufferFront;
    std::vector<float> m_analysisBufferBack;
    
    std::vector<float> m_yinBuffer;
    size_t m_writePos = 0;
    size_t m_analysisWritePos = 0;
    
    int m_analysisCounter = 0;
    
    float m_phase = 0.0f;
    float m_maxDelay = 1024.0f;
    
    std::atomic<float> m_detectedFreq{440.0f};
    std::atomic<bool> m_isVocalActive{false};

    std::thread m_workerThread;
    std::atomic<bool> m_workerRunning{false};
    bool m_hasWork = false; // Protected by m_mutex
    std::condition_variable m_cv;
    std::mutex m_mutex;
};

class AutoTuneNode : public FluxPlugin {
public:
    AutoTuneNode(int buf, float sr) : FluxPlugin("Auto-Tune", buf, sr) {
        addParam("Shift", 0.5f, 2.0f, 1.0f);
        addParam("Retune", 0.0f, 1.0f, 0.5f);
    }

    std::shared_ptr<FluxProcessor> createProcessor() override {
        return std::make_shared<AutoTuneProcessor>(getSampleRate());
    }
};

} // namespace Beam

#endif