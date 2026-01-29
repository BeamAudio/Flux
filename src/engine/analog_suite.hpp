#ifndef ANALOG_SUITE_HPP
#define ANALOG_SUITE_HPP

#include "flux_plugin.hpp"
#include "analog_base.hpp"
#include "biquad_filter_node.hpp"
#include <cmath>
#include <vector>
#include <array>
#include <random>
#include <algorithm>

namespace Beam {

// ============================================================================
// HELPERS
// ============================================================================

class SimpleReverb {
public:
    SimpleReverb(float sr) : m_sr(sr) {
        m_buffer.resize((size_t)(sr * 0.5f), 0.0f);
    }
    
    void setParams(float size, float decay, float mix) {
        m_feedback = std::clamp(decay, 0.0f, 0.98f);
        m_mix = mix;
    }

    float process(float in) {
        float out = m_buffer[m_readPos];
        float newVal = in + out * m_feedback;
        m_buffer[m_writePos] = newVal;
        
        if (++m_writePos >= m_buffer.size()) m_writePos = 0;
        if (++m_readPos >= m_buffer.size()) m_readPos = 0;
        
        return in * (1.0f - m_mix) + out * m_mix;
    }

private:
    float m_sr;
    std::vector<float> m_buffer;
    size_t m_writePos = 0;
    size_t m_readPos = 1000; 
    float m_feedback = 0.5f;
    float m_mix = 0.3f;
};

// ============================================================================
// 1. EQUALIZERS
// ============================================================================

class TubeP_EQ : public FluxPlugin {
public:
    TubeP_EQ(int buf, float sr) : FluxPlugin("Tube-P EQ", buf, sr) {
        addParam("Low Boost", 0.0f, 12.0f, 0.0f);
        addParam("Low Freq", 20.0f, 100.0f, 60.0f);
        addParam("High Boost", 0.0f, 12.0f, 0.0f);
        addParam("High Freq", 3000.0f, 16000.0f, 10000.0f);
        addParam("Tube Drive", 0.0f, 1.0f, 0.2f);
        m_lowShelf = std::make_unique<BiquadFilterNode>(FilterType::LowShelf, 60.0f, 0.707f, sr);
        m_highShelf = std::make_unique<BiquadFilterNode>(FilterType::HighShelf, 10000.0f, 0.707f, sr);
    }
    void processBlock(const float* in, float* out, int total) override {
        m_lowShelf->setCutoff(getParam("Low Freq"));
        m_lowShelf->setGain(getParam("Low Boost"));
        m_highShelf->setCutoff(getParam("High Freq"));
        m_highShelf->setGain(getParam("High Boost"));
        float drive = 1.0f + getParam("Tube Drive");
        
        std::copy(in, in + total, out);
        m_lowShelf->process(out, total / 2, 2);
        m_highShelf->process(out, total / 2, 2);
        for (int i = 0; i < total; ++i) out[i] = AnalogBase::saturateLangevin(out[i], drive);
    }
private:
    std::unique_ptr<BiquadFilterNode> m_lowShelf, m_highShelf;
};

class ConsoleE_EQ : public FluxPlugin {
public:
    ConsoleE_EQ(int buf, float sr) : FluxPlugin("Console-E", buf, sr) {
        addParam("LF Gain", -15.0f, 15.0f, 0.0f);
        addParam("LF Freq", 30.0f, 450.0f, 100.0f);
        addParam("LMF Gain", -15.0f, 15.0f, 0.0f);
        addParam("LMF Freq", 200.0f, 2500.0f, 1000.0f);
        addParam("HMF Gain", -15.0f, 15.0f, 0.0f);
        addParam("HMF Freq", 600.0f, 7000.0f, 3000.0f);
        addParam("HF Gain", -15.0f, 15.0f, 0.0f);
        addParam("HF Freq", 1500.0f, 16000.0f, 10000.0f);
        m_lf = std::make_unique<BiquadFilterNode>(FilterType::LowShelf, 100.0f, 0.707f, sr);
        m_lmf = std::make_unique<BiquadFilterNode>(FilterType::Peaking, 1000.0f, 1.0f, sr);
        m_hmf = std::make_unique<BiquadFilterNode>(FilterType::Peaking, 3000.0f, 1.0f, sr);
        m_hf = std::make_unique<BiquadFilterNode>(FilterType::HighShelf, 10000.0f, 0.707f, sr);
    }
    void processBlock(const float* in, float* out, int total) override {
        m_lf->setGain(getParam("LF Gain")); m_lf->setCutoff(getParam("LF Freq"));
        m_lmf->setGain(getParam("LMF Gain")); m_lmf->setCutoff(getParam("LMF Freq"));
        m_hmf->setGain(getParam("HMF Gain")); m_hmf->setCutoff(getParam("HMF Freq"));
        m_hf->setGain(getParam("HF Gain")); m_hf->setCutoff(getParam("HF Freq"));
        std::copy(in, in + total, out);
        m_lf->process(out, total / 2, 2);
        m_lmf->process(out, total / 2, 2);
        m_hmf->process(out, total / 2, 2);
        m_hf->process(out, total / 2, 2);
    }
private:
    std::unique_ptr<BiquadFilterNode> m_lf, m_lmf, m_hmf, m_hf;
};

class VintageG_EQ : public FluxPlugin {
public:
    VintageG_EQ(int buf, float sr) : FluxPlugin("Vintage-G", buf, sr) {
        addParam("Low Gain", -12.0f, 12.0f, 0.0f);
        addParam("Mid Gain", -12.0f, 12.0f, 0.0f);
        addParam("Mid Freq", 300.0f, 5000.0f, 1500.0f);
        addParam("High Gain", -12.0f, 12.0f, 0.0f);
        m_low = std::make_unique<BiquadFilterNode>(FilterType::LowShelf, 100.0f, 0.707f, sr);
        m_mid = std::make_unique<BiquadFilterNode>(FilterType::Peaking, 1500.0f, 0.707f, sr);
        m_high = std::make_unique<BiquadFilterNode>(FilterType::HighShelf, 10000.0f, 0.707f, sr);
    }
    void processBlock(const float* in, float* out, int total) override {
        m_low->setGain(getParam("Low Gain"));
        m_mid->setGain(getParam("Mid Gain")); m_mid->setCutoff(getParam("Mid Freq"));
        m_high->setGain(getParam("High Gain"));
        std::copy(in, in + total, out);
        m_low->process(out, total / 2, 2);
        m_mid->process(out, total / 2, 2);
        m_high->process(out, total / 2, 2);
    }
private:
    std::unique_ptr<BiquadFilterNode> m_low, m_mid, m_high;
};

class Graphic10_EQ : public FluxPlugin {
public:
    Graphic10_EQ(int buf, float sr) : FluxPlugin("Graphic-10", buf, sr) {
        m_freqs = {31, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
        for(float f : m_freqs) {
            std::string name = std::to_string((int)f) + "Hz";
            addParam(name, -12.0f, 12.0f, 0.0f);
            m_filters.push_back(std::make_unique<BiquadFilterNode>(FilterType::Peaking, f, 1.41f, sr));
        }
    }
    void processBlock(const float* in, float* out, int total) override {
        for(size_t i=0; i<m_filters.size(); ++i) {
            m_filters[i]->setGain(getParam(std::to_string((int)m_freqs[i]) + "Hz"));
        }
        std::copy(in, in + total, out);
        for(auto& f : m_filters) f->process(out, total / 2, 2);
    }
private:
    std::vector<std::unique_ptr<BiquadFilterNode>> m_filters;
    std::vector<float> m_freqs;
};

class AirLift_EQ : public FluxPlugin {
public:
    AirLift_EQ(int buf, float sr) : FluxPlugin("Air-Lift", buf, sr) {
        addParam("Air", 0.0f, 10.0f, 0.0f);
        addParam("Lift", 0.0f, 10.0f, 0.0f);
        m_air = std::make_unique<BiquadFilterNode>(FilterType::HighShelf, 20000.0f, 0.7f, sr);
        m_lift = std::make_unique<BiquadFilterNode>(FilterType::LowShelf, 80.0f, 0.7f, sr);
    }
    void processBlock(const float* in, float* out, int total) override {
        m_air->setGain(getParam("Air")); m_air->setCutoff(10000.0f + getParam("Air") * 500.0f);
        m_lift->setGain(getParam("Lift"));
        std::copy(in, in + total, out);
        m_lift->process(out, total / 2, 2);
        m_air->process(out, total / 2, 2);
    }
private:
    std::unique_ptr<BiquadFilterNode> m_air, m_lift;
};

// ============================================================================
// 2. DYNAMICS
// ============================================================================

class Opto2A : public FluxPlugin {
public:
    Opto2A(int buf, float sr) : FluxPlugin("Opto-2A", buf, sr) {
        addParam("Peak Redux", 0.0f, 100.0f, 0.0f);
        addParam("Gain", 0.0f, 40.0f, 30.0f);
        m_envelope = 0.0f;
    }
    void processBlock(const float* in, float* out, int total) override {
        float redux = getParam("Peak Redux") * 0.01f;
        float makeup = std::pow(10.0f, getParam("Gain") / 20.0f);
        for (int i = 0; i < total; ++i) {
            m_envelope = 0.9995f * m_envelope + 0.0005f * std::abs(in[i]);
            float gr = 1.0f / (1.0f + (m_envelope * redux * 10.0f));
            out[i] = std::tanh(in[i] * gr * makeup);
        }
    }
    float getLatestGR() const { return m_envelope; }
private:
    float m_envelope;
};

class FET76 : public FluxPlugin {
public:
    FET76(int buf, float sr) : FluxPlugin("FET-76", buf, sr) {
        addParam("Input", -20.0f, 20.0f, 0.0f);
        addParam("Ratio", 4.0f, 20.0f, 4.0f);
        addParam("Attack", 0.02f, 1.0f, 0.1f);
        m_envelope = 0.0f;
    }
    void processBlock(const float* in, float* out, int total) override {
        float inputGain = std::pow(10.0f, getParam("Input") / 20.0f);
        for (int i = 0; i < total; ++i) {
            m_envelope = 0.95f * m_envelope + 0.05f * std::abs(in[i] * inputGain); 
            float gr = 1.0f / (1.0f + m_envelope);
            out[i] = in[i] * inputGain * gr;
        }
    }
    float getLatestGR() const { return m_envelope; }
private:
    float m_envelope;
};

class VCABus : public FluxPlugin {
public:
    VCABus(int buf, float sr) : FluxPlugin("VCA-Bus", buf, sr) {
        addParam("Threshold", -40.0f, 0.0f, -10.0f);
        addParam("Ratio", 1.5f, 10.0f, 2.0f);
        addParam("Makeup", 0.0f, 20.0f, 0.0f);
        m_envelope = 0.0f;
    }
    void processBlock(const float* in, float* out, int total) override {
        float threshLin = std::pow(10.0f, getParam("Threshold") / 20.0f);
        float ratio = getParam("Ratio");
        float makeup = std::pow(10.0f, getParam("Makeup") / 20.0f);
        for (int i=0; i<total; ++i) {
            m_envelope = 0.9f * m_envelope + 0.1f * std::abs(in[i]);
            float gr = 1.0f;
            if (m_envelope > threshLin) {
                float overDB = 20.0f * std::log10(m_envelope / threshLin);
                gr = std::pow(10.0f, -(overDB * (1.0f - 1.0f/ratio)) / 20.0f);
            }
            out[i] = in[i] * gr * makeup;
        }
    }
    float getLatestGR() const { return m_envelope; }
private:
    float m_envelope;
};

class VariMu : public FluxPlugin {
public:
    VariMu(int buf, float sr) : FluxPlugin("Vari-Mu", buf, sr) {
        addParam("Input", 0.0f, 20.0f, 10.0f);
        addParam("Output", -10.0f, 10.0f, 0.0f);
        m_gr = 0.0f;
    }
    void processBlock(const float* in, float* out, int total) override {
        float inGain = std::pow(10.0f, getParam("Input") / 20.0f);
        float outGain = std::pow(10.0f, getParam("Output") / 20.0f);
        for (int i=0; i<total; ++i) {
            float s = in[i] * inGain;
            m_gr = 1.0f / (1.0f + std::abs(s) * 0.5f); 
            out[i] = s * m_gr * outGain;
        }
    }
    float getLatestGR() const { return 1.0f - m_gr; }
private:
    float m_gr;
};

// ============================================================================
// 3. SPACE (REVERB)
// ============================================================================

class SteelPlate : public FluxPlugin {
public:
    SteelPlate(int buf, float sr) : FluxPlugin("Steel Plate", buf, sr) {
        addParam("Decay", 0.1f, 5.0f, 2.0f);
        addParam("Mix", 0.0f, 1.0f, 0.3f);
        m_l = std::make_unique<SimpleReverb>(sr);
        m_r = std::make_unique<SimpleReverb>(sr);
    }
    void processBlock(const float* in, float* out, int total) override {
        m_l->setParams(1.0f, getParam("Decay")/5.0f, getParam("Mix"));
        m_r->setParams(1.0f, getParam("Decay")/5.0f, getParam("Mix"));
        for (int i = 0; i < total/2; ++i) {
            out[i*2] = m_l->process(in[i*2]);
            out[i*2+1] = m_r->process(in[i*2+1]);
        }
    }
private:
    std::unique_ptr<SimpleReverb> m_l, m_r;
};

class GoldenHall : public FluxPlugin {
public:
    GoldenHall(int buf, float sr) : FluxPlugin("Golden Hall", buf, sr) {
        addParam("Size", 1.0f, 10.0f, 5.0f);
        addParam("Mix", 0.0f, 1.0f, 0.4f);
        m_l = std::make_unique<SimpleReverb>(sr);
        m_r = std::make_unique<SimpleReverb>(sr);
    }
    void processBlock(const float* in, float* out, int total) override {
        m_l->setParams(getParam("Size"), 0.9f, getParam("Mix"));
        m_r->setParams(getParam("Size"), 0.9f, getParam("Mix"));
        for(int i=0; i<total/2; ++i) {
            out[i*2] = m_l->process(in[i*2]);
            out[i*2+1] = m_r->process(in[i*2+1]);
        }
    }
private:
    std::unique_ptr<SimpleReverb> m_l, m_r;
};

class CopperSpring : public FluxPlugin {
public:
    CopperSpring(int buf, float sr) : FluxPlugin("Copper Spring", buf, sr) {
        addParam("Tension", 0.0f, 1.0f, 0.5f);
        addParam("Mix", 0.0f, 1.0f, 0.3f);
        m_l = std::make_unique<SimpleReverb>(sr);
        m_r = std::make_unique<SimpleReverb>(sr);
    }
    void processBlock(const float* in, float* out, int total) override {
        m_l->setParams(2.0f, 0.8f, getParam("Mix"));
        m_r->setParams(2.0f, 0.8f, getParam("Mix"));
        for(int i=0; i<total/2; ++i) {
            out[i*2] = m_l->process(in[i*2]);
            out[i*2+1] = m_r->process(in[i*2+1]);
        }
    }
private:
    std::unique_ptr<SimpleReverb> m_l, m_r;
};

class Cathedral : public FluxPlugin {
public:
    Cathedral(int buf, float sr) : FluxPlugin("Cathedral", buf, sr) {
        addParam("Decay", 2.0f, 20.0f, 5.0f);
        addParam("Mix", 0.0f, 1.0f, 0.5f);
        m_l = std::make_unique<SimpleReverb>(sr);
        m_r = std::make_unique<SimpleReverb>(sr);
    }
    void processBlock(const float* in, float* out, int total) override {
        m_l->setParams(10.0f, 0.98f, getParam("Mix"));
        m_r->setParams(10.0f, 0.98f, getParam("Mix"));
        for(int i=0; i<total/2; ++i) {
            out[i*2] = m_l->process(in[i*2]);
            out[i*2+1] = m_r->process(in[i*2+1]);
        }
    }
private:
    std::unique_ptr<SimpleReverb> m_l, m_r;
};

class GrainVerb : public FluxPlugin {
public:
    GrainVerb(int buf, float sr) : FluxPlugin("Grain Verb", buf, sr) {
        addParam("Density", 0.0f, 1.0f, 0.5f);
        addParam("Mix", 0.0f, 1.0f, 0.3f);
        m_buffer.assign(44100, 0.0f);
    }
    void processBlock(const float* in, float* out, int total) override {
        float mix = getParam("Mix");
        static std::default_random_engine gen;
        std::uniform_int_distribution<int> dist(100, 44000);
        for(int i=0; i<total; ++i) {
            m_buffer[m_pos] = in[i];
            int tap = (m_pos - dist(gen) + 44100) % 44100;
            out[i] = in[i] * (1.0f - mix) + m_buffer[tap] * mix;
            if (++m_pos >= 44100) m_pos = 0;
        }
    }
private:
    std::vector<float> m_buffer;
    size_t m_pos = 0;
};

// ============================================================================
// 4. TIME (DELAY)
// ============================================================================

class EchoPlex : public FluxPlugin {
public:
    EchoPlex(int buf, float sr) : FluxPlugin("Echo-Plex", buf, sr) {
        addParam("Time", 0.1f, 2.0f, 0.5f);
        addParam("Feedback", 0.0f, 0.95f, 0.4f);
        addParam("Wow", 0.0f, 1.0f, 0.2f);
        m_buffer.assign((size_t)(sr * 2.0f), 0.0f);
        m_wf = std::make_unique<AnalogBase::WowFlutterGenerator>(sr);
    }
    void processBlock(const float* in, float* out, int total) override {
        float fb = getParam("Feedback");
        m_wf->setIntensity(getParam("Wow") * 0.01f, 0.0f);
        for (int i = 0; i < total; ++i) {
            float speedMod = m_wf->next();
            float delaySamps = getParam("Time") * getSampleRate() * (1.0f + speedMod);
            size_t readPos = (m_pos + m_buffer.size() - (size_t)delaySamps) % m_buffer.size();
            float delayOut = std::tanh(m_buffer[readPos]);
            m_buffer[m_pos] = in[i] + delayOut * fb;
            out[i] = in[i] + delayOut;
            if (++m_pos >= m_buffer.size()) m_pos = 0;
        }
    }
private:
    std::vector<float> m_buffer;
    size_t m_pos = 0;
    std::unique_ptr<AnalogBase::WowFlutterGenerator> m_wf;
};

class BBD_Bucket : public FluxPlugin {
public:
    BBD_Bucket(int buf, float sr) : FluxPlugin("BBD-Bucket", buf, sr) {
        addParam("Time", 0.01f, 0.5f, 0.1f);
        addParam("Darkness", 0.0f, 1.0f, 0.5f);
        m_buffer.assign((size_t)(sr * 1.0f), 0.0f);
        m_lpf = std::make_unique<BiquadFilterNode>(FilterType::LowPass, 2000.0f, 0.7f, sr);
    }
    void processBlock(const float* in, float* out, int total) override {
        m_lpf->setCutoff(10000.0f - getParam("Darkness") * 9000.0f);
        size_t delay = (size_t)(getParam("Time") * getSampleRate());
        for(int i=0; i<total; ++i) {
            size_t r = (m_pos + m_buffer.size() - delay) % m_buffer.size();
            float val = m_lpf->process(m_buffer[r]);
            m_buffer[m_pos] = in[i] + val * 0.4f;
            out[i] = in[i] + val;
            if (++m_pos >= m_buffer.size()) m_pos = 0;
        }
    }
private:
    std::vector<float> m_buffer;
    size_t m_pos=0;
    std::unique_ptr<BiquadFilterNode> m_lpf;
};

class Reverse_Delay : public FluxPlugin {
public:
    Reverse_Delay(int buf, float sr) : FluxPlugin("Reverse", buf, sr) {
        addParam("Mix", 0.0f, 1.0f, 0.5f);
        m_buffer.assign((size_t)sr, 0.0f);
    }
    void processBlock(const float* in, float* out, int total) override {
        float mix = getParam("Mix");
        for(int i=0; i<total; ++i) {
            m_buffer[m_pos] = in[i];
            size_t r = (m_buffer.size() - m_pos) % m_buffer.size();
            out[i] = in[i] * (1.0f - mix) + m_buffer[r] * mix;
            if (++m_pos >= m_buffer.size()) m_pos = 0;
        }
    }
private:
    std::vector<float> m_buffer;
    size_t m_pos = 0;
};

class PingPong_Delay : public FluxPlugin {
public:
    PingPong_Delay(int buf, float sr) : FluxPlugin("Ping-Pong", buf, sr) {
        addParam("Time", 0.1f, 1.0f, 0.4f);
        addParam("Feedback", 0.0f, 0.9f, 0.5f);
        m_l.assign((size_t)sr, 0.0f);
        m_r.assign((size_t)sr, 0.0f);
    }
    void processBlock(const float* in, float* out, int total) override {
        float fb = getParam("Feedback");
        size_t delay = (size_t)(getParam("Time") * getSampleRate());
        for(int i=0; i<total/2; ++i) {
            size_t r = (m_pos + m_l.size() - delay) % m_l.size();
            float dL = m_l[r], dR = m_r[r];
            m_l[m_pos] = in[i*2] + dR * fb;
            m_r[m_pos] = in[i*2+1] + dL * fb;
            out[i*2] = in[i*2] + dL;
            out[i*2+1] = in[i*2+1] + dR;
            if (++m_pos >= m_l.size()) m_pos = 0;
        }
    }
private:
    std::vector<float> m_l, m_r;
    size_t m_pos = 0;
};

class SpaceShift : public FluxPlugin {
public:
    SpaceShift(int buf, float sr) : FluxPlugin("Space Shift", buf, sr) {
        addParam("Width", 0.0f, 1.0f, 0.5f);
        addParam("Rate", 0.1f, 5.0f, 1.0f);
        m_buffer.assign(4000, 0.0f);
    }
    void processBlock(const float* in, float* out, int total) override {
        float width = getParam("Width") * 50.0f;
        float rate = getParam("Rate");
        for(int i=0; i<total; ++i) {
            m_buffer[m_pos] = in[i];
            m_phase += rate * 0.0001f; if(m_phase > 6.28f) m_phase -= 6.28f;
            float lfo = std::sin(m_phase) * width;
            size_t r = (m_pos + m_buffer.size() - 200 - (size_t)lfo) % m_buffer.size();
            out[i] = in[i] + m_buffer[r];
            if (++m_pos >= m_buffer.size()) m_pos = 0;
        }
    }
private:
    std::vector<float> m_buffer;
    size_t m_pos = 0;
    float m_phase = 0.0f;
};

// ============================================================================
// 5. LIMITER
// ============================================================================

class TubeLimiter : public FluxPlugin {
public:
    TubeLimiter(int buf, float sr) : FluxPlugin("Tube Limiter", buf, sr) {
        addParam("Threshold", -20.0f, 0.0f, 0.0f);
        addParam("Output", -10.0f, 0.0f, 0.0f);
    }
    void processBlock(const float* in, float* out, int total) override {
        float thresh = std::pow(10.0f, getParam("Threshold") / 20.0f);
        float ceiling = std::pow(10.0f, getParam("Output") / 20.0f);
        float peak = 0.0f;
        for (int i = 0; i < total; ++i) {
            float absS = std::abs(in[i]);
            if (absS > peak) peak = absS;
            out[i] = ((absS > thresh) ? (std::tanh(in[i] / thresh) * thresh) : in[i]) * ceiling;
        }
        m_gr = (peak > thresh) ? (peak - thresh) : 0.0f;
    }
    float getLatestGR() const { return m_gr; }
private:
    float m_gr = 0.0f;
};

// ============================================================================
// 6. UTILITY / METERING
// ============================================================================

class FluxSpectrumAnalyzer : public FluxPlugin {
public:
    FluxSpectrumAnalyzer(int buf, float sr) : FluxPlugin("Spectrum", buf, sr) {
        m_freqs = {31, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
        for(float f : m_freqs) {
            std::string name = std::to_string((int)f) + "Hz";
            addParam(name, -60.0f, 0.0f, -60.0f);
            m_filters.push_back(std::make_unique<BiquadFilterNode>(FilterType::Peaking, f, 4.0f, sr)); 
        }
    }
    void processBlock(const float* in, float* out, int total) override {
        std::copy(in, in + total, out);
        for(size_t b=0; b<m_filters.size(); ++b) {
            float peak = 0.0f;
            // Use strided processing for performance (check every 4th sample?)
            // Or full processing. Let's do full for accuracy.
            // Note: Single Biquad state is shared across interleaved channels here (mono sum analysis effectively due to state pollution if we don't reset or separate).
            // For a visualizer, mono sum is acceptable.
            for(int i=0; i<total; ++i) {
                float s = in[i];
                float band = m_filters[b]->process(s); 
                if (std::abs(band) > peak) peak = std::abs(band);
            }
            float currentDb = getParameter(std::to_string((int)m_freqs[b]) + "Hz")->getValue();
            float targetDb = (peak > 0.0001f) ? 20.0f * std::log10(peak) : -60.0f;
            float smooth = (targetDb > currentDb) ? 0.2f : 0.05f; // Fast attack, slow release
            getParameter(std::to_string((int)m_freqs[b]) + "Hz")->setValue(currentDb * (1.0f - smooth) + targetDb * smooth);
        }
    }

    std::vector<FluxNode::Port> getInputPorts() const override { return { {"In", 2} }; }
    std::vector<FluxNode::Port> getOutputPorts() const override { return { {"Out", 2} }; }

private:
    std::vector<std::unique_ptr<BiquadFilterNode>> m_filters;
    std::vector<float> m_freqs;
};

class FluxLoudnessMeter : public FluxPlugin {
public:
    FluxLoudnessMeter(int buf, float sr) : FluxPlugin("Loudness", buf, sr) {
        addParam("Momentary", -60.0f, 0.0f, -60.0f);
        addParam("ShortTerm", -60.0f, 0.0f, -60.0f);
        addParam("True Peak", -60.0f, 0.0f, -60.0f);
    }
    void processBlock(const float* in, float* out, int total) override {
        std::copy(in, in + total, out);
        float sumSq = 0.0f;
        float peak = 0.0f;
        for(int i=0; i<total; ++i) {
            float s = in[i];
            sumSq += s*s;
            peak = (std::max)(peak, std::abs(s));
        }
        float rms = std::sqrt(sumSq / (total + 1));
        float db = (rms > 0.0001f) ? 20.0f * std::log10(rms) : -60.0f;
        float peakDb = (peak > 0.0001f) ? 20.0f * std::log10(peak) : -60.0f;
        
        auto pM = getParameter("Momentary");
        if(pM) pM->setValue(pM->getValue() * 0.9f + db * 0.1f);
        auto pS = getParameter("ShortTerm");
        if(pS) pS->setValue(pS->getValue() * 0.995f + db * 0.005f);
        auto pT = getParameter("True Peak");
        if(pT) pT->setValue((std::max)(pT->getValue() - 0.5f, peakDb)); // Slow decay peak
    }
};

} // namespace Beam

#endif // ANALOG_SUITE_HPP
