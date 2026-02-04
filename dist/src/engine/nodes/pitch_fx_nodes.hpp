#ifndef PITCH_FX_NODES_HPP
#define PITCH_FX_NODES_HPP

#include "engine/plugins/flux_plugin.hpp"
#include "engine/dsp/flux_audio_utils.hpp"
#include "interface/editors/autotune_editor.hpp"
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
 * @brief High-performance real-time pitch corrector with scale-aware snapping.
 */
class AutoTuneProcessor : public FluxPluginProcessor {
public:
    AutoTuneProcessor(float sr, std::shared_ptr<MeterSource> ms) : FluxPluginProcessor(sr, ms), m_sampleRate(sr) {
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
            m_hasWork = true; 
        }
        m_cv.notify_all();
        if (m_workerThread.joinable()) m_workerThread.join();
    }

    void processBlock(const float* in, float* out, int total) override {
        float userShift = getParam(0); 
        float retuneSpeed = getParam(1); 
        float humanize = getParam(2);
        int key = (int)getParam(3);
        int scaleType = (int)getParam(4); // 0=Chromatic, 1=Major, 2=Minor
        
        float currentFreq = m_detectedFreq.load(std::memory_order_relaxed);
        bool active = m_isVocalActive.load(std::memory_order_relaxed) && currentFreq > 40.0f;

        for (int i = 0; i < total; ++i) {
            float sample = in[i];
            m_delayBuffer[m_writePos] = sample;
            
            // 1. Snapshot for worker thread
            m_analysisBufferFront[m_analysisWritePos] = sample;
            m_analysisWritePos = (m_analysisWritePos + 1) % m_analysisBufferFront.size();

            if (++m_analysisCounter >= 512) {
                m_analysisCounter = 0;
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
            float finalShift = userShift;

            if (active && retuneSpeed > 0.001f) {
                float targetFreq = snapToScale(currentFreq, key, scaleType);
                float autoShift = targetFreq / currentFreq;
                
                // Retune Speed smoothing
                float effectiveRetune = retuneSpeed;
                if (humanize > 0.1f) {
                    float diff = std::abs(1.0f - autoShift);
                    if (diff < 0.05f) effectiveRetune *= (1.0f - humanize);
                }

                m_smoothShift = m_smoothShift * (1.0f - effectiveRetune * 0.05f) + autoShift * (effectiveRetune * 0.05f);
                finalShift = userShift * m_smoothShift;

                if (i % 64 == 0) { // Throttle meter updates
                    float semitones = 12.0f * std::log2(currentFreq / 440.0f) + 69.0f;
                    float targetSemitones = 12.0f * std::log2(targetFreq / 440.0f) + 69.0f;
                    m_meterSource->updateMeter(0, std::fmod(semitones + 120.0f, 12.0f));
                    m_meterSource->updateMeter(1, std::fmod(targetSemitones + 120.0f, 12.0f));
                }
            } else {
                m_smoothShift = 1.0f;
                if (i % 64 == 0) {
                    m_meterSource->updateMeter(0, -1.0f);
                    m_meterSource->updateMeter(1, -1.0f);
                }
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
    float snapToScale(float freq, int key, int scaleType) {
        float semitones = 12.0f * std::log2(freq / 440.0f) + 69.0f;
        float relativeNote = std::fmod(semitones - (float)key + 120.0f, 12.0f);
        int octave = (int)std::floor((semitones - (float)key + 120.0f) / 12.0f) - 10;
        
        float snappedRelative = 0;
        if (scaleType == 0) { // Chromatic
            snappedRelative = std::round(relativeNote);
        } else if (scaleType == 1) { // Major
            int major[] = {0, 2, 4, 5, 7, 9, 11};
            snappedRelative = closestNote(relativeNote, major, 7);
        } else if (scaleType == 2) { // Minor
            int minor[] = {0, 2, 3, 5, 7, 8, 10};
            snappedRelative = closestNote(relativeNote, minor, 7);
        }

        float finalNote = (float)octave * 12.0f + snappedRelative + (float)key;
        return 440.0f * std::pow(2.0f, (finalNote - 69.0f) / 12.0f);
    }

    float closestNote(float note, int* scale, int len) {
        float bestDist = 100.0f;
        int bestNote = scale[0];
        for(int i=0; i<len; ++i) {
            float d = std::abs(note - (float)scale[i]);
            if (d < bestDist) { bestDist = d; bestNote = scale[i]; }
            float dWrap = std::abs(note - (float)(scale[i] + 12));
            if (dWrap < bestDist) { bestDist = dWrap; bestNote = scale[i] + 12; }
        }
        return (float)bestNote;
    }

    void workerLoop() {
        while (m_workerRunning) {
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [this] { return m_hasWork || !m_workerRunning; });
                m_hasWork = false;
            }
            if (!m_workerRunning) break;
            estimatePitchAndVAD(m_analysisBufferBack);
        }
    }

    void estimatePitchAndVAD(const std::vector<float>& buffer) {
        const int N = 600; 
        const float threshold = 0.15f;
        float energy = 0.0f;
        for (size_t i = 0; i < N; ++i) energy += buffer[i] * buffer[i];
        energy = std::sqrt(energy / (float)N);
        for (int tau = 0; tau < N; ++tau) m_yinBuffer[tau] = SIMD::yin_difference(buffer.data(), tau, N);

        m_yinBuffer[0] = 1.0f;
        float runningSum = 0.0f;
        for (int tau = 1; tau < N; ++tau) {
            runningSum += m_yinBuffer[tau];
            m_yinBuffer[tau] *= (float)tau / (runningSum + 1e-6f);
        }

        int tauFound = -1;
        float minVal = 1.0f;
        for (int tau = 20; tau < N; ++tau) {
            if (m_yinBuffer[tau] < threshold) { tauFound = tau; break; }
            if (m_yinBuffer[tau] < minVal) minVal = m_yinBuffer[tau];
        }

        float clarity = 1.0f - minVal;
        m_isVocalActive.store(energy > 0.01f && clarity > 0.6f, std::memory_order_relaxed);
        if (tauFound != -1) {
            float freq = m_sampleRate / (float)tauFound; 
            if (freq > 40.0f && freq < 2000.0f) m_detectedFreq.store(freq, std::memory_order_relaxed);
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
    std::vector<float> m_analysisBufferFront;
    std::vector<float> m_analysisBufferBack;
    std::vector<float> m_yinBuffer;
    size_t m_writePos = 0;
    size_t m_analysisWritePos = 0;
    int m_analysisCounter = 0;
    float m_phase = 0.0f;
    float m_maxDelay = 1024.0f;
    float m_smoothShift = 1.0f;
    std::atomic<float> m_detectedFreq{440.0f};
    std::atomic<bool> m_isVocalActive{false};
    std::thread m_workerThread;
    std::atomic<bool> m_workerRunning{false};
    bool m_hasWork = false; 
    std::condition_variable m_cv;
    std::mutex m_mutex;
};

class AutoTuneNode : public FluxPlugin {
public:
    AutoTuneNode(int buf, float sr) : FluxPlugin("Auto-Tune", buf, sr) {
        addParam("Transpose", 0.5f, 2.0f, 1.0f);
        addParam("Retune Speed", 0.0f, 1.0f, 0.5f);
        addParam("Humanize", 0.0f, 1.0f, 0.0f);
        addParam("Key", 0.0f, 11.0f, 0.0f);       
        addParam("Scale", 0.0f, 2.0f, 0.0f);     

        m_meterSource->addMeter("DetectedNote");
        m_meterSource->addMeter("TargetNote");
    }

    std::shared_ptr<FluxProcessor> createProcessor() override {
        return std::make_shared<AutoTuneProcessor>(getSampleRate(), getMeterSource());
    }

    void updateVisuals() override {
        m_lastDetectedNote = m_meterSource->getValue(0);
        m_lastTargetNote = m_meterSource->getValue(1);
    }

    float getLastDetectedNote() const { return m_lastDetectedNote; }
    float getLastTargetNote() const { return m_lastTargetNote; }

    std::shared_ptr<Component> createEditor(const NodeEditorContext& ctx) override {
        return std::make_shared<AutoTuneEditor>(this);
    }

private:
    float m_lastDetectedNote = -1.0f;
    float m_lastTargetNote = -1.0f;
};

} // namespace Beam

#endif