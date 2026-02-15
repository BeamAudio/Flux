#ifndef ANALOG_SUITE_HPP
#define ANALOG_SUITE_HPP

#include "sdk/beam_sdk.hpp"
#include "engine/nodes/analog_base.hpp"
#include "engine/nodes/biquad_filter_node.hpp"
#include "interface/editors/spectrum_editor.hpp"
#include "interface/editors/loudness_editor.hpp"
#include <cmath>
#include <vector>
#include <array>
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

class TubeP_EQ : public Beam::SDK::BeamPlugin {
public:
    TubeP_EQ(int, float sr) : BeamPlugin("Tube-P EQ", "EQ") {
        setPanelStyle(SDK::stylePultec());
        lowBoost = &addFloatParam("Low Boost", 0.0f, 12.0f, 0.0f);
        lowFreq = &addFloatParam("Low Freq", 20.0f, 100.0f, 60.0f);
        highBoost = &addFloatParam("High Boost", 0.0f, 12.0f, 0.0f);
        highFreq = &addFloatParam("High Freq", 3000.0f, 16000.0f, 10000.0f);
        tubeDrive = &addFloatParam("Tube Drive", 0.0f, 1.0f, 0.2f);
        for(int i=0; i<2; ++i) {
            m_lowShelf[i] = std::make_unique<BiquadFilterNode>(FilterType::LowShelf, 60.0f, 0.707f, sr);
            m_highShelf[i] = std::make_unique<BiquadFilterNode>(FilterType::HighShelf, 10000.0f, 0.707f, sr);
        }
    }
    void process(float** io, int frames) override {
        float drive = 1.0f + tubeDrive->getValue();
        for(int ch=0; ch<2; ++ch) {
            float* data = io[ch]; if(!data) continue;
            m_lowShelf[ch]->setCutoff(lowFreq->getValue()); m_lowShelf[ch]->setGain(lowBoost->getValue());
            m_highShelf[ch]->setCutoff(highFreq->getValue()); m_highShelf[ch]->setGain(highBoost->getValue());
            m_lowShelf[ch]->process(data, frames, 1);
            m_highShelf[ch]->process(data, frames, 1);
            for(int i=0; i<frames; ++i) data[i] = AnalogBase::saturateLangevin(data[i], drive);
        }
    }
    void prepareToPlay(float sr, int) override {
        for(int i=0; i<2; ++i) {
            m_lowShelf[i] = std::make_unique<BiquadFilterNode>(FilterType::LowShelf, 60.0f, 0.707f, sr);
            m_highShelf[i] = std::make_unique<BiquadFilterNode>(FilterType::HighShelf, 10000.0f, 0.707f, sr);
        }
    }
private:
    Parameter *lowBoost, *lowFreq, *highBoost, *highFreq, *tubeDrive;
    std::unique_ptr<BiquadFilterNode> m_lowShelf[2], m_highShelf[2];
};
REGISTER_BEAM_PLUGIN(TubeP_EQ)

class ConsoleE_EQ : public Beam::SDK::BeamPlugin {
public:
    ConsoleE_EQ(int, float sr) : BeamPlugin("Console-E", "EQ") {
        setPanelStyle(SDK::styleSSL());
        lfGain = &addFloatParam("LF Gain", -15.0f, 15.0f, 0.0f);
        lfFreq = &addFloatParam("LF Freq", 30.0f, 450.0f, 100.0f);
        lmfGain = &addFloatParam("LMF Gain", -15.0f, 15.0f, 0.0f);
        lmfFreq = &addFloatParam("LMF Freq", 200.0f, 2500.0f, 1000.0f);
        hmfGain = &addFloatParam("HMF Gain", -15.0f, 15.0f, 0.0f);
        hmfFreq = &addFloatParam("HMF Freq", 600.0f, 7000.0f, 3000.0f);
        hfGain = &addFloatParam("HF Gain", -15.0f, 15.0f, 0.0f);
        hfFreq = &addFloatParam("HF Freq", 1500.0f, 16000.0f, 10000.0f);
        for(int i=0; i<2; ++i) {
            m_f[i][0] = std::make_unique<BiquadFilterNode>(FilterType::LowShelf, 100.0f, 0.707f, sr);
            m_f[i][1] = std::make_unique<BiquadFilterNode>(FilterType::Peaking, 1000.0f, 1.0f, sr);
            m_f[i][2] = std::make_unique<BiquadFilterNode>(FilterType::Peaking, 3000.0f, 1.0f, sr);
            m_f[i][3] = std::make_unique<BiquadFilterNode>(FilterType::HighShelf, 10000.0f, 0.707f, sr);
        }
    }
    void process(float** io, int frames) override {
        for(int ch=0; ch<2; ++ch) {
            float* data = io[ch]; if(!data) continue;
            m_f[ch][0]->setGain(lfGain->getValue()); m_f[ch][0]->setCutoff(lfFreq->getValue());
            m_f[ch][1]->setGain(lmfGain->getValue()); m_f[ch][1]->setCutoff(lmfFreq->getValue());
            m_f[ch][2]->setGain(hmfGain->getValue()); m_f[ch][2]->setCutoff(hmfFreq->getValue());
            m_f[ch][3]->setGain(hfGain->getValue()); m_f[ch][3]->setCutoff(hfFreq->getValue());
            for(int i=0; i<4; ++i) m_f[ch][i]->process(data, frames, 1);
        }
    }
    void prepareToPlay(float sr, int) override {
        for(int i=0; i<2; ++i) {
            m_f[i][0] = std::make_unique<BiquadFilterNode>(FilterType::LowShelf, 100.0f, 0.707f, sr);
            m_f[i][1] = std::make_unique<BiquadFilterNode>(FilterType::Peaking, 1000.0f, 1.0f, sr);
            m_f[i][2] = std::make_unique<BiquadFilterNode>(FilterType::Peaking, 3000.0f, 1.0f, sr);
            m_f[i][3] = std::make_unique<BiquadFilterNode>(FilterType::HighShelf, 10000.0f, 0.707f, sr);
        }
    }
private:
    Parameter *lfGain, *lfFreq, *lmfGain, *lmfFreq, *hmfGain, *hmfFreq, *hfGain, *hfFreq;
    std::unique_ptr<BiquadFilterNode> m_f[2][4];
};
REGISTER_BEAM_PLUGIN(ConsoleE_EQ)



class VintageG_EQ : public Beam::SDK::BeamPlugin {
public:
    VintageG_EQ(int, float sr) : BeamPlugin("Vintage-G", "EQ") {
        auto style = SDK::stylePultec(); style.title = "VINTAGE-G"; style.chassisColor = {0.8f, 0.8f, 0.7f, 1.0f}; style.textColor = {0.1f, 0.1f, 0.1f, 1.0f};
        setPanelStyle(style);
        lowGain = &addFloatParam("Low Gain", -12.0f, 12.0f, 0.0f);
        midGain = &addFloatParam("Mid Gain", -12.0f, 12.0f, 0.0f);
        midFreq = &addFloatParam("Mid Freq", 300.0f, 5000.0f, 1500.0f);
        highGain = &addFloatParam("High Gain", -12.0f, 12.0f, 0.0f);
        for(int i=0; i<2; ++i) {
            m_low[i] = std::make_unique<BiquadFilterNode>(FilterType::LowShelf, 100.0f, 0.707f, sr);
            m_mid[i] = std::make_unique<BiquadFilterNode>(FilterType::Peaking, 1500.0f, 0.707f, sr);
            m_high[i] = std::make_unique<BiquadFilterNode>(FilterType::HighShelf, 10000.0f, 0.707f, sr);
        }
    }
    void process(float** io, int frames) override {
        for(int ch=0; ch<2; ++ch) {
            float* data = io[ch]; if(!data) continue;
            m_low[ch]->setGain(lowGain->getValue());
            m_mid[ch]->setGain(midGain->getValue()); m_mid[ch]->setCutoff(midFreq->getValue());
            m_high[ch]->setGain(highGain->getValue());
            m_low[ch]->process(data, frames, 1);
            m_mid[ch]->process(data, frames, 1);
            m_high[ch]->process(data, frames, 1);
        }
    }
    void prepareToPlay(float sr, int) override {
        for(int i=0; i<2; ++i) {
            m_low[i] = std::make_unique<BiquadFilterNode>(FilterType::LowShelf, 100.0f, 0.707f, sr);
            m_mid[i] = std::make_unique<BiquadFilterNode>(FilterType::Peaking, 1500.0f, 0.707f, sr);
            m_high[i] = std::make_unique<BiquadFilterNode>(FilterType::HighShelf, 10000.0f, 0.707f, sr);
        }
    }
private:
    Parameter *lowGain, *midGain, *midFreq, *highGain;
    std::unique_ptr<BiquadFilterNode> m_low[2], m_mid[2], m_high[2];
};
REGISTER_BEAM_PLUGIN(VintageG_EQ)

class Graphic10_EQ : public Beam::SDK::BeamPlugin {
public:
    Graphic10_EQ(int, float sr) : BeamPlugin("Graphic-10", "EQ") {
        auto style = SDK::styleSSL(); style.title = "GRAPHIC-10"; 
        setPanelStyle(style);
        std::vector<float> freqs = {31, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
        for(float f : freqs) {
            m_gains.push_back(&addFloatParam(std::to_string((int)f) + "Hz", -12.0f, 12.0f, 0.0f));
            for(int ch=0; ch<2; ++ch) m_filters[ch].push_back(std::make_unique<BiquadFilterNode>(FilterType::Peaking, f, 1.41f, sr));
        }
    }
    void process(float** io, int frames) override {
        for(int ch=0; ch<2; ++ch) {
            float* data = io[ch]; if(!data) continue;
            for(size_t i=0; i < m_filters[ch].size(); ++i) {
                m_filters[ch][i]->setGain(m_gains[i]->getValue());
                m_filters[ch][i]->process(data, frames, 1);
            }
        }
    }
    void prepareToPlay(float sr, int) override {
        std::vector<float> freqs = {31, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
        for(int ch=0; ch<2; ++ch) {
            m_filters[ch].clear();
            for(float f : freqs) m_filters[ch].push_back(std::make_unique<BiquadFilterNode>(FilterType::Peaking, f, 1.41f, sr));
        }
    }
private:
    std::vector<Parameter*> m_gains;
    std::vector<std::unique_ptr<BiquadFilterNode>> m_filters[2];
};
REGISTER_BEAM_PLUGIN(Graphic10_EQ)

class AirLift_EQ : public Beam::SDK::BeamPlugin {
public:
    AirLift_EQ(int, float sr) : BeamPlugin("Air-Lift", "EQ") {
        auto style = SDK::styleSSL(); style.title = "AIR-LIFT"; style.chassisColor = {0.9f, 0.9f, 0.95f, 1.0f}; style.textColor = {0.1f, 0.2f, 0.5f, 1.0f};
        setPanelStyle(style);
        air = &addFloatParam("Air", 0.0f, 10.0f, 0.0f);
        lift = &addFloatParam("Lift", 0.0f, 10.0f, 0.0f);
        for(int i=0; i<2; ++i) {
            m_air[i] = std::make_unique<BiquadFilterNode>(FilterType::HighShelf, 20000.0f, 0.7f, sr);
            m_lift[i] = std::make_unique<BiquadFilterNode>(FilterType::LowShelf, 80.0f, 0.7f, sr);
        }
    }
    void process(float** io, int frames) override {
        for(int ch=0; ch<2; ++ch) {
            float* data = io[ch]; if(!data) continue;
            m_air[ch]->setGain(air->getValue()); m_air[ch]->setCutoff(10000.0f + air->getValue() * 500.0f);
            m_lift[ch]->setGain(lift->getValue());
            m_lift[ch]->process(data, frames, 1);
            m_air[ch]->process(data, frames, 1);
        }
    }
    void prepareToPlay(float sr, int) override {
        for(int i=0; i<2; ++i) {
            m_air[i] = std::make_unique<BiquadFilterNode>(FilterType::HighShelf, 20000.0f, 0.7f, sr);
            m_lift[i] = std::make_unique<BiquadFilterNode>(FilterType::LowShelf, 80.0f, 0.7f, sr);
        }
    }
private:
    Parameter *air, *lift;
    std::unique_ptr<BiquadFilterNode> m_air[2], m_lift[2];
};
REGISTER_BEAM_PLUGIN(AirLift_EQ)

// ============================================================================
// 2. DYNAMICS
// ============================================================================

class Opto2A : public SDK::BeamPlugin {
public:
    Opto2A(int, float sr) : BeamPlugin("Opto-2A", "Dynamics") {
        auto style = SDK::stylePultec(); style.title = "OPTO-2A"; style.chassisColor = {0.8f, 0.8f, 0.82f, 1.0f}; style.textColor = {0.1f, 0.1f, 0.1f, 1.0f};
        setPanelStyle(style);
        peakRedux = &addFloatParam("Peak Redux", 0.0f, 100.0f, 0.0f);
        gain = &addFloatParam("Gain", 0.0f, 40.0f, 30.0f);
        useSidechain(true);
        addMeter("GR");
    }
    void process(float** io, int frames) override {
        process(io, io, frames); // Use self as sidechain if not provided
    }
    void process(float** io, float** sc, int frames) override {
        float* L = io[0]; float* R = io[1];
        float* scL = sc[0]; float* scR = sc[1];
        float redux = peakRedux->getValue() * 0.01f;
        float makeup = std::pow(10.0f, gain->getValue() / 20.0f);
        float maxGR = 0.0f;
        for (int i = 0; i < frames; ++i) {
            float detector = (std::abs(scL[i]) + (scR ? std::abs(scR[i]) : std::abs(scL[i]))) * 0.5f;
            m_envelope = 0.9995f * m_envelope + 0.0005f * detector;
            float gr = 1.0f / (1.0f + (m_envelope * redux * 10.0f));
            if (L) L[i] = std::tanh(L[i] * gr * makeup);
            if (R) R[i] = std::tanh(R[i] * gr * makeup);
            maxGR = (std::max)(maxGR, 1.0f - gr);
        }
        updateMeter(0, maxGR);
    }
private:
    Parameter *peakRedux, *gain;
    float m_envelope = 0.0f;
};
REGISTER_BEAM_PLUGIN(Opto2A)

class FET76 : public SDK::BeamPlugin {
public:
    FET76(int, float sr) : BeamPlugin("FET-76", "Dynamics") {
        setPanelStyle(SDK::styleFET());
        input = &addFloatParam("Input", -20.0f, 20.0f, 0.0f);
        ratio = &addFloatParam("Ratio", 4.0f, 20.0f, 4.0f);
        attack = &addFloatParam("Attack", 0.02f, 1.0f, 0.1f);
        useSidechain(true);
        addMeter("GR");
    }
    void process(float** io, int frames) override {
        process(io, io, frames);
    }
    void process(float** io, float** sc, int frames) override {
        float* L = io[0]; float* R = io[1];
        float* scL = sc[0]; float* scR = sc[1];
        float inputGain = std::pow(10.0f, input->getValue() / 20.0f);
        float alpha = attack->getValue() * 0.1f;
        float maxGR = 0.0f;
        for (int i = 0; i < frames; ++i) {
            float detector = (std::abs(scL[i]) + (scR ? std::abs(scR[i]) : std::abs(scL[i]))) * 0.5f * inputGain;
            m_envelope = (1.0f - alpha) * m_envelope + alpha * detector; 
            float gr = 1.0f / (1.0f + m_envelope);
            if (L) L[i] = L[i] * inputGain * gr;
            if (R) R[i] = R[i] * inputGain * gr;
            maxGR = (std::max)(maxGR, 1.0f - gr);
        }
        updateMeter(0, maxGR);
    }
private:
    Parameter *input, *ratio, *attack;
    float m_envelope = 0.0f;
};
REGISTER_BEAM_PLUGIN(FET76)

// ----------------------------------------------------------------------------

class VariMu : public SDK::BeamPlugin {
public:
    VariMu(int, float sr) : BeamPlugin("Vari-Mu", "Dynamics") {
        auto style = SDK::stylePultec(); style.title = "VARI-MU"; style.chassisColor = {0.1f, 0.1f, 0.12f, 1.0f}; style.textColor = {0.9f, 0.8f, 0.2f, 1.0f};
        setPanelStyle(style);
        input = &addFloatParam("Input", 0.0f, 20.0f, 10.0f);
        output = &addFloatParam("Output", -10.0f, 10.0f, 0.0f);
        addMeter("GR");
    }
    void process(float** io, int frames) override {
        float inGain = std::pow(10.0f, input->getValue() / 20.0f);
        float outGain = std::pow(10.0f, output->getValue() / 20.0f);
        float maxGR = 0.0f;
        for (int i = 0; i < frames; ++i) {
            for(int ch=0; ch<2; ++ch) {
                if(!io[ch]) continue;
                float s = io[ch][i] * inGain;
                float gr = 1.0f / (1.0f + std::abs(s) * 0.5f); 
                io[ch][i] = s * gr * outGain;
                maxGR = (std::max)(maxGR, 1.0f - gr);
            }
        }
        updateMeter(0, maxGR);
    }
private:
    Parameter *input, *output;
};
REGISTER_BEAM_PLUGIN(VariMu)

// ============================================================================
// 3. SPACE (REVERB)
// ============================================================================

class SteelPlate : public SDK::BeamPlugin {
public:
    SteelPlate(int, float sr) : BeamPlugin("Steel Plate", "Reverb") {
        auto style = SDK::stylePultec(); style.title = "STEEL PLATE"; style.chassisColor = {0.6f, 0.62f, 0.65f, 1.0f};
        setPanelStyle(style);
        decay = &addFloatParam("Decay", 0.1f, 5.0f, 2.0f);
        mix = &addFloatParam("Mix", 0.0f, 1.0f, 0.3f);
        m_l = std::make_unique<SimpleReverb>(sr);
        m_r = std::make_unique<SimpleReverb>(sr);
    }
    void process(float** io, int frames) override {
        m_l->setParams(1.0f, decay->getValue()/5.0f, mix->getValue());
        m_r->setParams(1.0f, decay->getValue()/5.0f, mix->getValue());
        for (int i = 0; i < frames; ++i) {
            if (io[0]) io[0][i] = m_l->process(io[0][i]);
            if (io[1]) io[1][i] = m_r->process(io[1][i]);
        }
    }
    void prepareToPlay(float sr, int) override {
        m_l = std::make_unique<SimpleReverb>(sr);
        m_r = std::make_unique<SimpleReverb>(sr);
    }
private:
    Parameter *decay, *mix;
    std::unique_ptr<SimpleReverb> m_l, m_r;
};
REGISTER_BEAM_PLUGIN(SteelPlate)

class GoldenHall : public SDK::BeamPlugin {
public:
    GoldenHall(int, float sr) : BeamPlugin("Golden Hall", "Reverb") {
        auto style = SDK::stylePultec(); style.title = "GOLDEN HALL"; style.chassisColor = {0.7f, 0.6f, 0.3f, 1.0f}; style.textColor = {0.2f, 0.1f, 0.0f, 1.0f};
        setPanelStyle(style);
        size = &addFloatParam("Size", 1.0f, 10.0f, 5.0f);
        mix = &addFloatParam("Mix", 0.0f, 1.0f, 0.4f);
        m_l = std::make_unique<SimpleReverb>(sr);
        m_r = std::make_unique<SimpleReverb>(sr);
    }
    void process(float** io, int frames) override {
        m_l->setParams(size->getValue(), 0.9f, mix->getValue());
        m_r->setParams(size->getValue(), 0.9f, mix->getValue());
        for (int i = 0; i < frames; ++i) {
            if (io[0]) io[0][i] = m_l->process(io[0][i]);
            if (io[1]) io[1][i] = m_r->process(io[1][i]);
        }
    }
    void prepareToPlay(float sr, int) override {
        m_l = std::make_unique<SimpleReverb>(sr);
        m_r = std::make_unique<SimpleReverb>(sr);
    }
private:
    Parameter *size, *mix;
    std::unique_ptr<SimpleReverb> m_l, m_r;
};
REGISTER_BEAM_PLUGIN(GoldenHall)

class EchoPlex : public SDK::BeamPlugin {
public:
    EchoPlex(int, float sr) : BeamPlugin("Echo-Plex", "Delay") {
        auto style = SDK::styleFET(); style.title = "ECHO-PLEX";
        setPanelStyle(style);
        time = &addFloatParam("Time", 0.1f, 2.0f, 0.5f);
        feedback = &addFloatParam("Feedback", 0.0f, 0.95f, 0.4f);
        wow = &addFloatParam("Wow", 0.0f, 1.0f, 0.2f);
        m_buffer.assign((size_t)(sr * 2.1f), 0.0f);
        m_wf = std::make_unique<AnalogBase::WowFlutterGenerator>(sr);
    }
    void process(float** io, int frames) override {
        float* L = io[0]; float* R = io[1];
        float fb = feedback->getValue(); float t = time->getValue();
        m_wf->setIntensity(wow->getValue() * 0.01f, 0.0f);
        for (int i = 0; i < frames; ++i) {
            float speedMod = m_wf->next();
            float delaySamps = t * 44100.0f * (1.0f + speedMod);
            size_t r = (m_pos + m_buffer.size() - (size_t)delaySamps) % m_buffer.size();
            float val = std::tanh(m_buffer[r]);
            float input = (L[i] + (R ? R[i] : L[i])) * 0.5f;
            m_buffer[m_pos] = input + val * fb;
            if (L) L[i] += val; if (R) R[i] += val;
            if (++m_pos >= m_buffer.size()) m_pos = 0;
        }
    }
    void prepareToPlay(float sr, int) override {
        m_buffer.assign((size_t)(sr * 2.1f), 0.0f);
        m_wf = std::make_unique<AnalogBase::WowFlutterGenerator>(sr);
    }
private:
    Parameter *time, *feedback, *wow;
    std::vector<float> m_buffer;
    size_t m_pos = 0;
    std::unique_ptr<AnalogBase::WowFlutterGenerator> m_wf;
};
REGISTER_BEAM_PLUGIN(EchoPlex)

class BBD_Bucket : public SDK::BeamPlugin {
public:
    BBD_Bucket(int, float sr) : BeamPlugin("BBD-Bucket", "Delay") {
        auto style = SDK::styleFET(); style.title = "BBD-BUCKET"; style.chassisColor = {0.1f, 0.1f, 0.2f, 1.0f}; style.textColor = {0.6f, 0.8f, 1.0f, 1.0f};
        setPanelStyle(style);
        time = &addFloatParam("Time", 0.01f, 0.5f, 0.1f);
        darkness = &addFloatParam("Darkness", 0.0f, 1.0f, 0.5f);
        for(int i=0; i<2; ++i) {
            m_buffer[i].assign((size_t)(sr * 1.0f), 0.0f);
            m_lpf[i] = std::make_unique<BiquadFilterNode>(FilterType::LowPass, 2000.0f, 0.7f, sr);
        }
    }
    void process(float** io, int frames) override {
        m_lpf[0]->setCutoff(10000.0f - darkness->getValue() * 9000.0f);
        m_lpf[1]->setCutoff(10000.0f - darkness->getValue() * 9000.0f);
        size_t dS = (size_t)(time->getValue() * 44100.0f);
        for(int i=0; i<frames; ++i) {
            for(int ch=0; ch<2; ++ch) {
                if(!io[ch]) continue;
                size_t r = (m_pos + m_buffer[ch].size() - dS) % m_buffer[ch].size();
                float val = m_lpf[ch]->process(m_buffer[ch][r]);
                m_buffer[ch][m_pos] = io[ch][i] + val * 0.4f;
                io[ch][i] += val;
            }
            if (++m_pos >= m_buffer[0].size()) m_pos = 0;
        }
    }
private:
    Parameter *time, *darkness;
    std::vector<float> m_buffer[2];
    size_t m_pos = 0;
    std::unique_ptr<BiquadFilterNode> m_lpf[2];
};
REGISTER_BEAM_PLUGIN(BBD_Bucket)

class Reverse_Delay : public SDK::BeamPlugin {
public:
    Reverse_Delay(int, float sr) : BeamPlugin("Reverse", "Delay") {
        mix = &addFloatParam("Mix", 0.0f, 1.0f, 0.5f);
        for(int i=0; i<2; ++i) m_buffer[i].assign((size_t)sr, 0.0f);
    }
    void process(float** io, int frames) override {
        float m = mix->getValue();
        for(int i=0; i<frames; ++i) {
            for(int ch=0; ch<2; ++ch) {
                if(!io[ch]) continue;
                m_buffer[ch][m_pos] = io[ch][i];
                size_t r = (m_buffer[ch].size() - m_pos) % m_buffer[ch].size();
                io[ch][i] = io[ch][i] * (1.0f - m) + m_buffer[ch][r] * m;
            }
            if (++m_pos >= m_buffer[0].size()) m_pos = 0;
        }
    }
private:
    Parameter* mix;
    std::vector<float> m_buffer[2];
    size_t m_pos = 0;
};
REGISTER_BEAM_PLUGIN(Reverse_Delay)

class PingPong_Delay : public SDK::BeamPlugin {
public:
    PingPong_Delay(int, float sr) : BeamPlugin("Ping-Pong", "Delay") {
        time = &addFloatParam("Time", 0.1f, 1.0f, 0.4f);
        feedback = &addFloatParam("Feedback", 0.0f, 0.9f, 0.5f);
        for(int i=0; i<2; ++i) m_buffer[i].assign((size_t)sr, 0.0f);
    }
    void process(float** io, int frames) override {
        float fb = feedback->getValue();
        size_t delay = (size_t)(time->getValue() * 44100.0f);
        for(int i=0; i<frames; ++i) {
            size_t r = (m_pos + m_buffer[0].size() - delay) % m_buffer[0].size();
            float dL = m_buffer[0][r], dR = m_buffer[1][r];
            m_buffer[0][m_pos] = io[0][i] + dR * fb;
            m_buffer[1][m_pos] = (io[1] ? io[1][i] : io[0][i]) + dL * fb;
            io[0][i] += dL; if (io[1]) io[1][i] += dR;
            if (++m_pos >= m_buffer[0].size()) m_pos = 0;
        }
    }
private:
    Parameter *time, *feedback;
    std::vector<float> m_buffer[2];
    size_t m_pos = 0;
};
REGISTER_BEAM_PLUGIN(PingPong_Delay)

class SpaceShift : public SDK::BeamPlugin {
public:
    SpaceShift(int, float sr) : BeamPlugin("Space Shift", "Modulation") {
        auto style = SDK::styleFET(); style.title = "SPACE SHIFT"; style.chassisColor = {0.2f, 0.0f, 0.2f, 1.0f}; style.textColor = {0.8f, 0.6f, 1.0f, 1.0f};
        setPanelStyle(style);
        width = &addFloatParam("Width", 0.0f, 1.0f, 0.5f);
        rate = &addFloatParam("Rate", 0.1f, 5.0f, 1.0f);
        m_buffer[0].assign(4000, 0.0f); m_buffer[1].assign(4000, 0.0f);
    }
    void process(float** io, int frames) override {
        float w = width->getValue() * 50.0f; float r_val = rate->getValue();
        for(int i=0; i<frames; ++i) {
            m_phase += r_val * 0.0001f; if(m_phase > 6.28f) m_phase -= 6.28f;
            float lfo = std::sin(m_phase) * w;
            for(int ch=0; ch<2; ++ch) {
                if(!io[ch]) continue;
                m_buffer[ch][m_pos] = io[ch][i];
                size_t r = (m_pos + m_buffer[ch].size() - 200 - (size_t)lfo) % m_buffer[ch].size();
                io[ch][i] += m_buffer[ch][r];
            }
            if (++m_pos >= m_buffer[0].size()) m_pos = 0;
        }
    }
private:
    Parameter *width, *rate;
    std::vector<float> m_buffer[2];
    size_t m_pos = 0; float m_phase = 0;
};
REGISTER_BEAM_PLUGIN(SpaceShift)

// ============================================================================
// 5. LIMITER
// ============================================================================

class TubeLimiter : public SDK::BeamPlugin {
public:
    TubeLimiter(int, float sr) : BeamPlugin("Tube Limiter", "Dynamics") {
        auto style = SDK::styleFET(); style.title = "LIMITER"; style.chassisColor = {0.6f, 0.2f, 0.2f, 1.0f};
        setPanelStyle(style);
        threshold = &addFloatParam("Threshold", -20.0f, 0.0f, 0.0f);
        output = &addFloatParam("Output", -10.0f, 0.0f, 0.0f);
        addMeter("GR");
    }
    void process(float** io, int frames) override {
        float thresh = std::pow(10.0f, threshold->getValue() / 20.0f);
        float ceiling = std::pow(10.0f, output->getValue() / 20.0f);
        float peak = 0.0f;
        for (int i = 0; i < frames; ++i) {
            for(int ch=0; ch<2; ++ch) {
                if(!io[ch]) continue;
                float absS = std::abs(io[ch][i]);
                if (absS > peak) peak = absS;
                io[ch][i] = ((absS > thresh) ? (std::tanh(io[ch][i] / thresh) * thresh) : io[ch][i]) * ceiling;
            }
        }
        float gr = (peak > thresh) ? (peak - thresh) * 2.0f : 0.0f;
        updateMeter(0, std::clamp(gr, 0.0f, 1.0f));
    }
private:
    Parameter *threshold, *output;
};
REGISTER_BEAM_PLUGIN(TubeLimiter)

// ============================================================================
// 6. UTILITY / METERING
// ============================================================================

class FluxSpectrumProcessor : public FluxPluginProcessor {
public:
    FluxSpectrumProcessor(float sr) {
        std::vector<float> freqs = {31, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
        for(float f : freqs) m_filters.push_back(std::make_unique<BiquadFilterNode>(FilterType::Peaking, f, 4.0f, sr));
        m_levels.fill(0.0f); // Initialize levels
    }
    void process(const float** inputs, float** outputs, int frames) override {
        const float* in = inputs[0];
        float* out = outputs[0];
        std::copy(in, in + frames * 2, out);
        
        // Use mutable params for visualizers? 
        // Actually, the Processor should NOT write to the Node's parameters directly.
        // It provides its own output buffer or atomic metrics.
        // For visualizers, we'll store the values in the processor and have the Node fetch them.
        for(size_t b=0; b<m_filters.size(); ++b) {
            float peak = 0.0f;
            for(int i=0; i<frames*2; ++i) {
                float band = m_filters[b]->process(in[i]); 
                peak = (std::max)(peak, std::abs(band));
            }
            m_levels[b] = peak;
        }
    }
    void processBlock(const float* in, float* out, int total) override {
        const float* inputs[] = { in, nullptr };
        float* outputs[] = { out };
        process(inputs, outputs, total / 2);
    }
    float getLevel(int band) const { return m_levels[band]; }
private:
    std::vector<std::unique_ptr<BiquadFilterNode>> m_filters;
    std::array<float, 10> m_levels;
};

class FluxSpectrumAnalyzer : public FluxPlugin {
public:
    FluxSpectrumAnalyzer(int buf, float sr) : FluxPlugin("Spectrum", buf, sr), m_sr(sr) {
        std::vector<float> freqs = {31, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
        for(float f : freqs) addParam(std::to_string((int)f) + "Hz", -60.0f, 0.0f, -60.0f);
    }
    std::shared_ptr<FluxProcessor> createProcessor() override { 
        auto p = std::make_shared<FluxSpectrumProcessor>(m_sr);
        m_lastProc = p.get();
        return p;
    }
    // Visualizer update logic (called by UI)
    void updateVisuals() {
        if (!m_lastProc) return;
        for (int i=0; i<10; ++i) {
            float peak = m_lastProc->getLevel(i);
            
            // Reconstruct name exactly as in constructor: 31Hz, 63Hz, etc.
            std::string name = std::to_string((int)(31.25f * std::pow(2.0f, (float)i))) + "Hz";
            
            auto p = getParameter(name);
            if (!p) continue;

            float currentDb = p->getValue();
            float targetDb = (peak > 0.0001f) ? 20.0f * std::log10(peak) : -60.0f;
            float smooth = (targetDb > currentDb) ? 0.2f : 0.05f;
            
            p->setValue(currentDb * (1.0f - smooth) + targetDb * smooth);
        }
    }

    std::vector<FluxNode::Port> getInputPorts() const override { return { {"In", 2} }; }
    std::vector<FluxNode::Port> getOutputPorts() const override { return { {"Out", 2} }; }

private:
    float m_sr;
    FluxSpectrumProcessor* m_lastProc = nullptr;
};

class FluxLoudnessProcessor : public FluxPluginProcessor {
public:
    void processBlock(const float* in, float* out, int total) override {
        std::copy(in, in + total, out);
        float sumSq = 0.0f;
        float peak = 0.0f;
        for(int i=0; i<total; ++i) {
            sumSq += in[i]*in[i];
            peak = (std::max)(peak, std::abs(in[i]));
        }
        m_rms = std::sqrt(sumSq / (total + 1));
        m_peak = peak;
    }
    float getRMS() const { return m_rms; }
    float getPeak() const { return m_peak; }
private:
    float m_rms = 0.0f, m_peak = 0.0f;
};

class FluxLoudnessMeter : public FluxPlugin {
public:
    FluxLoudnessMeter(int buf, float sr) : FluxPlugin("Loudness", buf, sr) {
        addParam("Momentary", -60.0f, 0.0f, -60.0f);
        addParam("ShortTerm", -60.0f, 0.0f, -60.0f);
        addParam("True Peak", -60.0f, 0.0f, -60.0f);
    }
    std::shared_ptr<FluxProcessor> createProcessor() override { 
        auto p = std::make_shared<FluxLoudnessProcessor>();
        m_lastProc = p.get();
        return p; 
    }
    void updateVisuals() {
        if (!m_lastProc) return;
        float db = (m_lastProc->getRMS() > 0.0001f) ? 20.0f * std::log10(m_lastProc->getRMS()) : -60.0f;
        float peakDb = (m_lastProc->getPeak() > 0.0001f) ? 20.0f * std::log10(m_lastProc->getPeak()) : -60.0f;
        
        if (auto p = getParameter("Momentary")) p->setValue(p->getValue() * 0.9f + db * 0.1f);
        if (auto p = getParameter("ShortTerm")) p->setValue(p->getValue() * 0.995f + db * 0.005f);
        if (auto p = getParameter("True Peak")) p->setValue((std::max)(p->getValue() - 0.5f, peakDb));
    }
    std::shared_ptr<Component> createEditor(const NodeEditorContext&) override { return std::make_shared<LoudnessEditor>(this); }
private:
    FluxLoudnessProcessor* m_lastProc = nullptr;
};

/**
 * @class Saturation
 * @brief Example of the simplified BeamEngine FX API.
 */
class Saturation : public SDK::BeamPlugin {
public:
    Saturation(int, float sr) : BeamPlugin("Saturation", "Utilities") {
        auto style = SDK::styleFET(); style.title = "SATURATION"; style.chassisColor = {0.5f, 0.1f, 0.0f, 1.0f};
        setPanelStyle(style);
        drive = &addFloatParam("Drive", 0.0f, 10.0f, 1.0f);
    }
    void process(float** io, int frames) override {
        float* L = io[0]; float* R = io[1];
        float d = drive->getValue();
        for(int i=0; i<frames; ++i) {
            if (L) L[i] = std::tanh(L[i] * d);
            if (R) R[i] = std::tanh(R[i] * d);
        }
    }
private:
    Parameter* drive;
};
REGISTER_BEAM_PLUGIN(Saturation)

class LookaheadLimiter : public SDK::BeamPlugin {
public:
    LookaheadLimiter(int, float sr) : BeamPlugin("Lookahead Limiter", "Dynamics"), m_sr(sr) {
        auto style = SDK::styleFET(); style.title = "L2 LIMITER"; style.chassisColor = {0.1f, 0.1f, 0.1f, 1.0f}; style.textColor = {1.0f, 0.8f, 0.0f, 1.0f};
        setPanelStyle(style);
        threshold = &addFloatParam("Threshold", -20.0f, 0.0f, 0.0f);
        ceiling = &addFloatParam("Ceiling", -10.0f, 0.0f, -0.1f);
        addMeter("GR");
        prepareToPlay(sr, 512);
    }
    size_t getLatency() const override { return (size_t)(0.005f * m_sr); }
    void process(float** io, int frames) override {
        float* L = io[0]; float* R = io[1];
        float thresh = std::pow(10.0f, threshold->getValue() / 20.0f);
        float ceilVal = std::pow(10.0f, ceiling->getValue() / 20.0f);
        float maxGR = 0.0f;
        for (int i = 0; i < frames; ++i) {
            for (int ch = 0; ch < 2; ++ch) {
                float* data = io[ch];
                if (!data) continue;
                float input = data[i];
                float delayed = m_buffer[ch][m_pos];
                m_buffer[ch][m_pos] = input;
                float peak = std::abs(input); 
                float gr = (peak > thresh) ? (thresh / peak) : 1.0f;
                data[i] = delayed * gr * ceilVal;
                maxGR = (std::max)(maxGR, 1.0f - gr);
            }
            m_pos = (m_pos + 1) % m_buffer[0].size();
        }
        updateMeter(0, maxGR);
    }
    void prepareToPlay(float sr, int) override {
        m_sr = sr;
        size_t lookahead = (size_t)(0.005f * sr);
        m_buffer[0].assign(lookahead, 0.0f);
        m_buffer[1].assign(lookahead, 0.0f);
        m_pos = 0;
    }
private:
    Parameter *threshold, *ceiling;
    std::vector<float> m_buffer[2];
    size_t m_pos = 0;
    float m_sr;
};
REGISTER_BEAM_PLUGIN(LookaheadLimiter)

} // namespace Beam
#endif // ANALOG_SUITE_HPP
