#ifndef ANALOG_SUITE_HPP
#define ANALOG_SUITE_HPP

#include "engine/plugins/flux_plugin.hpp"
#include "engine/nodes/analog_base.hpp"
#include "engine/nodes/biquad_filter_node.hpp"
#include "interface/analog/analog_ui_templates.hpp"
#include "interface/editors/spectrum_editor.hpp"
#include "interface/editors/loudness_editor.hpp"
#include "interface/editors/dynamics_editor.hpp" // Keep for fallback?
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

class TubeP_EQProcessor : public FluxPluginProcessor {
public:
    TubeP_EQProcessor(float sr) {
        m_lowShelf = std::make_unique<BiquadFilterNode>(FilterType::LowShelf, 60.0f, 0.707f, sr);
        m_highShelf = std::make_unique<BiquadFilterNode>(FilterType::HighShelf, 10000.0f, 0.707f, sr);
    }
    void processBlock(const float* in, float* out, int total) override {
        m_lowShelf->setCutoff(getParam(1)); m_lowShelf->setGain(getParam(0));
        m_highShelf->setCutoff(getParam(3)); m_highShelf->setGain(getParam(2));
        float drive = 1.0f + getParam(4);
        std::copy(in, in + total, out);
        m_lowShelf->process(out, total / 2, 2);
        m_highShelf->process(out, total / 2, 2);
        for (int i = 0; i < total; ++i) out[i] = AnalogBase::saturateLangevin(out[i], drive);
    }
private:
    std::unique_ptr<BiquadFilterNode> m_lowShelf, m_highShelf;
};

class TubeP_EQ : public FluxPlugin {
public:
    TubeP_EQ(int buf, float sr) : FluxPlugin("Tube-P EQ", buf, sr), m_sr(sr) {
        addParam("Low Boost", 0.0f, 12.0f, 0.0f);
        addParam("Low Freq", 20.0f, 100.0f, 60.0f);
        addParam("High Boost", 0.0f, 12.0f, 0.0f);
        addParam("High Freq", 3000.0f, 16000.0f, 10000.0f);
        addParam("Tube Drive", 0.0f, 1.0f, 0.2f);
    }
    std::shared_ptr<FluxProcessor> createProcessor() override { return std::make_shared<TubeP_EQProcessor>(m_sr); }
    std::shared_ptr<Component> createEditor(const NodeEditorContext&) override { return std::make_shared<RackUnitUI>(this, RackUnitUI::Pultec()); }
private:
    float m_sr;
};

class ConsoleE_EQProcessor : public FluxPluginProcessor {
public:
    ConsoleE_EQProcessor(float sr) {
        m_lf = std::make_unique<BiquadFilterNode>(FilterType::LowShelf, 100.0f, 0.707f, sr);
        m_lmf = std::make_unique<BiquadFilterNode>(FilterType::Peaking, 1000.0f, 1.0f, sr);
        m_hmf = std::make_unique<BiquadFilterNode>(FilterType::Peaking, 3000.0f, 1.0f, sr);
        m_hf = std::make_unique<BiquadFilterNode>(FilterType::HighShelf, 10000.0f, 0.707f, sr);
    }
    void processBlock(const float* in, float* out, int total) override {
        m_lf->setGain(getParam(0)); m_lf->setCutoff(getParam(1));
        m_lmf->setGain(getParam(2)); m_lmf->setCutoff(getParam(3));
        m_hmf->setGain(getParam(4)); m_hmf->setCutoff(getParam(5));
        m_hf->setGain(getParam(6)); m_hf->setCutoff(getParam(7));
        std::copy(in, in + total, out);
        m_lf->process(out, total / 2, 2);
        m_lmf->process(out, total / 2, 2);
        m_hmf->process(out, total / 2, 2);
        m_hf->process(out, total / 2, 2);
    }
private:
    std::unique_ptr<BiquadFilterNode> m_lf, m_lmf, m_hmf, m_hf;
};

class ConsoleE_EQ : public FluxPlugin {
public:
    ConsoleE_EQ(int buf, float sr) : FluxPlugin("Console-E", buf, sr), m_sr(sr) {
        addParam("LF Gain", -15.0f, 15.0f, 0.0f);
        addParam("LF Freq", 30.0f, 450.0f, 100.0f);
        addParam("LMF Gain", -15.0f, 15.0f, 0.0f);
        addParam("LMF Freq", 200.0f, 2500.0f, 1000.0f);
        addParam("HMF Gain", -15.0f, 15.0f, 0.0f);
        addParam("HMF Freq", 600.0f, 7000.0f, 3000.0f);
        addParam("HF Gain", -15.0f, 15.0f, 0.0f);
        addParam("HF Freq", 1500.0f, 16000.0f, 10000.0f);
    }
    std::shared_ptr<FluxProcessor> createProcessor() override { return std::make_shared<ConsoleE_EQProcessor>(m_sr); }
    std::shared_ptr<Component> createEditor(const NodeEditorContext&) override { return std::make_shared<RackUnitUI>(this, RackUnitUI::SSL()); }
private:
    float m_sr;
};



class VintageG_EQProcessor : public FluxPluginProcessor {
public:
    VintageG_EQProcessor(float sr) {
        m_low = std::make_unique<BiquadFilterNode>(FilterType::LowShelf, 100.0f, 0.707f, sr);
        m_mid = std::make_unique<BiquadFilterNode>(FilterType::Peaking, 1500.0f, 0.707f, sr);
        m_high = std::make_unique<BiquadFilterNode>(FilterType::HighShelf, 10000.0f, 0.707f, sr);
    }
    void processBlock(const float* in, float* out, int total) override {
        m_low->setGain(getParam(0));
        m_mid->setGain(getParam(1)); m_mid->setCutoff(getParam(2));
        m_high->setGain(getParam(3));
        std::copy(in, in + total, out);
        m_low->process(out, total / 2, 2);
        m_mid->process(out, total / 2, 2);
        m_high->process(out, total / 2, 2);
    }
private:
    std::unique_ptr<BiquadFilterNode> m_low, m_mid, m_high;
};

class VintageG_EQ : public FluxPlugin {
public:
    VintageG_EQ(int buf, float sr) : FluxPlugin("Vintage-G", buf, sr), m_sr(sr) {
        addParam("Low Gain", -12.0f, 12.0f, 0.0f);
        addParam("Mid Gain", -12.0f, 12.0f, 0.0f);
        addParam("Mid Freq", 300.0f, 5000.0f, 1500.0f);
        addParam("High Gain", -12.0f, 12.0f, 0.0f);
    }
    std::shared_ptr<FluxProcessor> createProcessor() override { return std::make_shared<VintageG_EQProcessor>(m_sr); }
    std::shared_ptr<Component> createEditor(const NodeEditorContext&) override { 
        auto style = RackUnitUI::Pultec(); style.title = "VINTAGE-G"; style.chassisColor = {0.8f, 0.8f, 0.7f, 1.0f}; style.textColor = {0.1f, 0.1f, 0.1f, 1.0f};
        return std::make_shared<RackUnitUI>(this, style); 
    }
private:
    float m_sr;
};

class Graphic10_EQProcessor : public FluxPluginProcessor {
public:
    Graphic10_EQProcessor(float sr) {
        std::vector<float> freqs = {31, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
        for(float f : freqs) m_filters.push_back(std::make_unique<BiquadFilterNode>(FilterType::Peaking, f, 1.41f, sr));
    }
    void processBlock(const float* in, float* out, int total) override {
        for(int i=0; i < (int)m_filters.size(); ++i) m_filters[i]->setGain(getParam(i));
        std::copy(in, in + total, out);
        for(auto& f : m_filters) f->process(out, total / 2, 2);
    }
private:
    std::vector<std::unique_ptr<BiquadFilterNode>> m_filters;
};

class Graphic10_EQ : public FluxPlugin {
public:
    Graphic10_EQ(int buf, float sr) : FluxPlugin("Graphic-10", buf, sr), m_sr(sr) {
        std::vector<float> freqs = {31, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
        for(float f : freqs) addParam(std::to_string((int)f) + "Hz", -12.0f, 12.0f, 0.0f);
    }
    std::shared_ptr<FluxProcessor> createProcessor() override { return std::make_shared<Graphic10_EQProcessor>(m_sr); }
    std::shared_ptr<Component> createEditor(const NodeEditorContext&) override { 
        auto style = RackUnitUI::API(); style.title = "GRAPHIC-10"; 
        return std::make_shared<RackUnitUI>(this, style); 
    }
private:
    float m_sr;
};

class AirLift_EQProcessor : public FluxPluginProcessor {
public:
    AirLift_EQProcessor(float sr) {
        m_air = std::make_unique<BiquadFilterNode>(FilterType::HighShelf, 20000.0f, 0.7f, sr);
        m_lift = std::make_unique<BiquadFilterNode>(FilterType::LowShelf, 80.0f, 0.7f, sr);
    }
    void processBlock(const float* in, float* out, int total) override {
        m_air->setGain(getParam(0)); m_air->setCutoff(10000.0f + getParam(0) * 500.0f);
        m_lift->setGain(getParam(1));
        std::copy(in, in + total, out);
        m_lift->process(out, total / 2, 2);
        m_air->process(out, total / 2, 2);
    }
private:
    std::unique_ptr<BiquadFilterNode> m_air, m_lift;
};

class AirLift_EQ : public FluxPlugin {
public:
    AirLift_EQ(int buf, float sr) : FluxPlugin("Air-Lift", buf, sr), m_sr(sr) {
        addParam("Air", 0.0f, 10.0f, 0.0f);
        addParam("Lift", 0.0f, 10.0f, 0.0f);
    }
    std::shared_ptr<FluxProcessor> createProcessor() override { return std::make_shared<AirLift_EQProcessor>(m_sr); }
    std::shared_ptr<Component> createEditor(const NodeEditorContext&) override { 
        auto style = RackUnitUI::API(); style.title = "AIR-LIFT"; style.chassisColor = {0.9f, 0.9f, 0.95f, 1.0f}; style.textColor = {0.1f, 0.2f, 0.5f, 1.0f};
        return std::make_shared<RackUnitUI>(this, style); 
    }
private:
    float m_sr;
};

// ============================================================================
// 2. DYNAMICS
// ============================================================================

class Opto2AProcessor : public FluxPluginProcessor {
public:
    Opto2AProcessor(std::shared_ptr<MeterSource> meterSource) : m_meterSource(meterSource), m_envelope(0.0f) {}

    void process(const float** inputs, float** outputs, int frames) override {
        const float* in = inputs[0];
        const float* sc = (inputs[1] != nullptr) ? inputs[1] : inputs[0]; // Sidechain fallback
        float* out = outputs[0];

        float redux = getParam(0) * 0.01f;
        float makeup = std::pow(10.0f, getParam(1) / 20.0f);
        float maxGR = 0.0f;

        for (int i = 0; i < frames * 2; ++i) {
            float detector = std::abs(sc[i]);
            m_envelope = 0.9995f * m_envelope + 0.0005f * detector;
            float gr = 1.0f / (1.0f + (m_envelope * redux * 10.0f));
            out[i] = std::tanh(in[i] * gr * makeup);
            if (1.0f - gr > maxGR) maxGR = 1.0f - gr;
        }
        
        if (m_meterSource) m_meterSource->updateMeter(0, maxGR);
        m_lastGR = maxGR;
    }

    float getGR() const { return m_lastGR; }

    void processBlock(const float* in, float* out, int total) override {
        const float* inputs[] = { in, nullptr };
        float* outputs[] = { out };
        process(inputs, outputs, total / 2);
    }
private:
    std::shared_ptr<MeterSource> m_meterSource;
    float m_envelope;
    float m_lastGR = 0.0f;
};

class Opto2A : public FluxPlugin {
public:
    Opto2A(int buf, float sr) : FluxPlugin("Opto-2A", buf, sr) {
        addParam("Peak Redux", 0.0f, 100.0f, 0.0f);
        addParam("Gain", 0.0f, 40.0f, 30.0f);
        m_meterSource->addMeter("GR");
    }
    std::vector<Port> getInputPorts() const override { return { {"Input", 2}, {"Sidechain", 2, Port::Sidechain} }; }
    std::shared_ptr<FluxProcessor> createProcessor() override { 
        auto p = std::make_shared<Opto2AProcessor>(m_meterSource); 
        m_lastProc = p.get();
        return p; 
    }
    float getLatestGR() const { return m_lastProc ? m_lastProc->getGR() : 0.0f; }
    std::shared_ptr<Component> createEditor(const NodeEditorContext& ctx) override {
        auto style = RackUnitUI::Utility("OPTO-2A"); style.chassisColor = {0.8f, 0.8f, 0.82f, 1.0f}; style.textColor = {0.1f, 0.1f, 0.1f, 1.0f}; style.showMeter = true;
        return std::make_shared<RackUnitUI>(this, style);
    }
private:
    Opto2AProcessor* m_lastProc = nullptr;
};

class FET76Processor : public FluxPluginProcessor {
public:
    FET76Processor(std::shared_ptr<MeterSource> meterSource) : m_meterSource(meterSource), m_envelope(0.0f) {}
    void process(const float** inputs, float** outputs, int frames) override {
        const float* in = inputs[0];
        const float* sc = (inputs[1] != nullptr) ? inputs[1] : inputs[0];
        float* out = outputs[0];
        float inputGain = std::pow(10.0f, getParam(0) / 20.0f);
        float alpha = getParam(2) * 0.1f;
        float maxGR = 0.0f;
        for (int i = 0; i < frames * 2; ++i) {
            float detector = std::abs(sc[i] * inputGain);
            m_envelope = (1.0f - alpha) * m_envelope + alpha * detector; 
            float gr = 1.0f / (1.0f + m_envelope);
            out[i] = in[i] * inputGain * gr;
            if (1.0f - gr > maxGR) maxGR = 1.0f - gr;
        }
        if (m_meterSource) m_meterSource->updateMeter(0, maxGR);
        m_lastGR = maxGR;
    }
    float getGR() const { return m_lastGR; }
    void processBlock(const float* in, float* out, int total) override {
        const float* inputs[] = { in, nullptr };
        float* outputs[] = { out };
        process(inputs, outputs, total / 2);
    }
private:
    std::shared_ptr<MeterSource> m_meterSource;
    float m_envelope;
    float m_lastGR = 0.0f;
};

class FET76 : public FluxPlugin {
public:
    FET76(int buf, float sr) : FluxPlugin("FET-76", buf, sr) {
        addParam("Input", -20.0f, 20.0f, 0.0f);
        addParam("Ratio", 4.0f, 20.0f, 4.0f);
        addParam("Attack", 0.02f, 1.0f, 0.1f);
        m_meterSource->addMeter("GR");
    }
    std::vector<Port> getInputPorts() const override { return { {"Input", 2}, {"Sidechain", 2, Port::Sidechain} }; }
    std::shared_ptr<FluxProcessor> createProcessor() override { 
        auto p = std::make_shared<FET76Processor>(m_meterSource);
        m_lastProc = p.get();
        return p;
    }
    float getLatestGR() const { return m_lastProc ? m_lastProc->getGR() : 0.0f; }
    std::shared_ptr<Component> createEditor(const NodeEditorContext& ctx) override {
        auto style = RackUnitUI::FET();
        return std::make_shared<RackUnitUI>(this, style);
    }
private:
    FET76Processor* m_lastProc = nullptr;
};

class VCABusProcessor : public FluxPluginProcessor {
public:
    VCABusProcessor(std::shared_ptr<MeterSource> meterSource) : m_meterSource(meterSource), m_envelope(0.0f) {}

    void updateParameters(const float* params) override {
        m_threshold = params[0];
        m_ratio = params[1];
        m_makeup = params[2];
        // Attack/Release fixed or not exposed in param list?
    }

    void process(const float** inputs, float** outputs, int frames) override {
        const float* in = inputs[0]; 
        float* out = outputs[0];
        processBlock(in, out, frames * 2);
    }

    void processBlock(const float* in, float* out, int total) override {
        float threshLin = std::pow(10.0f, m_threshold / 20.0f);
        float makeupLin = std::pow(10.0f, m_makeup / 20.0f);
        float maxGR = 0.0f;
        
        for (int i=0; i<total; ++i) {
            m_envelope = 0.9f * m_envelope + 0.1f * std::abs(in[i]);
            float gr = 1.0f;
            if (m_envelope > threshLin) {
                float overDB = 20.0f * std::log10(m_envelope / threshLin);
                gr = std::pow(10.0f, -(overDB * (1.0f - 1.0f/m_ratio)) / 20.0f);
            }
            out[i] = in[i] * gr * makeupLin;
            if (1.0f - gr > maxGR) maxGR = 1.0f - gr;
        }
        if (m_meterSource) m_meterSource->updateMeter(0, maxGR);
        m_lastGR = maxGR;
    }
    float getGR() const { return m_lastGR; }
private:
    std::shared_ptr<MeterSource> m_meterSource;
    float m_envelope;
    float m_lastGR = 0.0f;
    float m_threshold = -10.0f;
    float m_ratio = 2.0f;
    float m_makeup = 0.0f;
};

class VCABus : public FluxPlugin {
public:
    VCABus(int buf, float sr) : FluxPlugin("VCA-Bus", buf, sr) {
        addParam("Threshold", -40.0f, 0.0f, -10.0f);
        addParam("Ratio", 1.5f, 10.0f, 2.0f);
        addParam("Makeup", 0.0f, 20.0f, 0.0f);
        m_meterSource->addMeter("GR");
    }
    std::shared_ptr<FluxProcessor> createProcessor() override { 
        auto p = std::make_shared<VCABusProcessor>(m_meterSource);
        m_lastProc = p.get();
        return p;
    }
    float getLatestGR() const { return m_lastProc ? m_lastProc->getGR() : 0.0f; }
    std::shared_ptr<Component> createEditor(const NodeEditorContext& ctx) override { 
        auto style = RackUnitUI::SSL(); style.title = "VCA-BUS"; style.showMeter = true;
        return std::make_shared<RackUnitUI>(this, style); 
    }
private:
    VCABusProcessor* m_lastProc = nullptr;
};

class VariMuProcessor : public FluxPluginProcessor {
public:
    VariMuProcessor(std::shared_ptr<MeterSource> meterSource) : m_meterSource(meterSource), m_gr(1.0f) {}
    void processBlock(const float* in, float* out, int total) override {
        float inGain = std::pow(10.0f, getParam(0) / 20.0f);
        float outGain = std::pow(10.0f, getParam(1) / 20.0f);
        float maxGR = 0.0f;
        for (int i=0; i<total; ++i) {
            float s = in[i] * inGain;
            float gr = 1.0f / (1.0f + std::abs(s) * 0.5f); 
            out[i] = s * gr * outGain;
            if (1.0f - gr > maxGR) maxGR = 1.0f - gr;
        }
        if (m_meterSource) m_meterSource->updateMeter(0, maxGR);
        m_lastGR = maxGR;
    }
    float getGR() const { return m_lastGR; }
private:
    std::shared_ptr<MeterSource> m_meterSource;
    float m_gr; 
    float m_lastGR = 0.0f;
};

class VariMu : public FluxPlugin {
public:
    VariMu(int buf, float sr) : FluxPlugin("Vari-Mu", buf, sr) {
        addParam("Input", 0.0f, 20.0f, 10.0f);
        addParam("Output", -10.0f, 10.0f, 0.0f);
        m_meterSource->addMeter("GR");
    }
    std::shared_ptr<FluxProcessor> createProcessor() override { 
        auto p = std::make_shared<VariMuProcessor>(m_meterSource);
        m_lastProc = p.get();
        return p;
    }
    float getLatestGR() const { return m_lastProc ? m_lastProc->getGR() : 0.0f; }
    std::shared_ptr<Component> createEditor(const NodeEditorContext& ctx) override { 
        auto style = RackUnitUI::Pultec(); style.title = "VARI-MU"; style.chassisColor = {0.1f, 0.1f, 0.12f, 1.0f}; style.textColor = {0.9f, 0.8f, 0.2f, 1.0f}; style.showMeter = true;
        return std::make_shared<RackUnitUI>(this, style); 
    }
private:
    VariMuProcessor* m_lastProc = nullptr;
};

// ============================================================================
// 3. SPACE (REVERB)
// ============================================================================

class SteelPlateProcessor : public FluxPluginProcessor {
public:
    SteelPlateProcessor(float sr) {
        m_l = std::make_unique<SimpleReverb>(sr);
        m_r = std::make_unique<SimpleReverb>(sr);
    }
    void processBlock(const float* in, float* out, int total) override {
        m_l->setParams(1.0f, getParam(0)/5.0f, getParam(1));
        m_r->setParams(1.0f, getParam(0)/5.0f, getParam(1));
        for (int i = 0; i < total/2; ++i) {
            out[i*2] = m_l->process(in[i*2]);
            out[i*2+1] = m_r->process(in[i*2+1]);
        }
    }
private:
    std::unique_ptr<SimpleReverb> m_l, m_r;
};

class SteelPlate : public FluxPlugin {
public:
    SteelPlate(int buf, float sr) : FluxPlugin("Steel Plate", buf, sr), m_sr(sr) {
        addParam("Decay", 0.1f, 5.0f, 2.0f);
        addParam("Mix", 0.0f, 1.0f, 0.3f);
    }
    std::shared_ptr<FluxProcessor> createProcessor() override { return std::make_shared<SteelPlateProcessor>(m_sr); }
    std::shared_ptr<Component> createEditor(const NodeEditorContext&) override { 
        auto style = RackUnitUI::Reverb("STEEL PLATE"); style.chassisColor = {0.6f, 0.62f, 0.65f, 1.0f}; 
        return std::make_shared<RackUnitUI>(this, style); 
    }
private:
    float m_sr;
};

class GoldenHallProcessor : public FluxPluginProcessor {
public:
    GoldenHallProcessor(float sr) {
        m_l = std::make_unique<SimpleReverb>(sr);
        m_r = std::make_unique<SimpleReverb>(sr);
    }
    void processBlock(const float* in, float* out, int total) override {
        m_l->setParams(getParam(0), 0.9f, getParam(1));
        m_r->setParams(getParam(0), 0.9f, getParam(1));
        for(int i=0; i<total/2; ++i) {
            out[i*2] = m_l->process(in[i*2]);
            out[i*2+1] = m_r->process(in[i*2+1]);
        }
    }
private:
    std::unique_ptr<SimpleReverb> m_l, m_r;
};

class GoldenHall : public FluxPlugin {
public:
    GoldenHall(int buf, float sr) : FluxPlugin("Golden Hall", buf, sr), m_sr(sr) {
        addParam("Size", 1.0f, 10.0f, 5.0f);
        addParam("Mix", 0.0f, 1.0f, 0.4f);
    }
    std::shared_ptr<FluxProcessor> createProcessor() override { return std::make_shared<GoldenHallProcessor>(m_sr); }
    std::shared_ptr<Component> createEditor(const NodeEditorContext&) override { 
        auto style = RackUnitUI::Reverb("GOLDEN HALL"); style.chassisColor = {0.7f, 0.6f, 0.3f, 1.0f}; style.textColor = {0.2f, 0.1f, 0.0f, 1.0f};
        return std::make_shared<RackUnitUI>(this, style); 
    }
private:
    float m_sr;
};

class CopperSpringProcessor : public FluxPluginProcessor {
public:
    CopperSpringProcessor(float sr) {
        m_l = std::make_unique<SimpleReverb>(sr);
        m_r = std::make_unique<SimpleReverb>(sr);
    }
    void processBlock(const float* in, float* out, int total) override {
        m_l->setParams(2.0f, 0.8f, getParam(1));
        m_r->setParams(2.0f, 0.8f, getParam(1));
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
    CopperSpring(int buf, float sr) : FluxPlugin("Copper Spring", buf, sr), m_sr(sr) {
        addParam("Tension", 0.0f, 1.0f, 0.5f);
        addParam("Mix", 0.0f, 1.0f, 0.3f);
    }
    std::shared_ptr<FluxProcessor> createProcessor() override { return std::make_shared<CopperSpringProcessor>(m_sr); }
    std::shared_ptr<Component> createEditor(const NodeEditorContext&) override { 
        auto style = RackUnitUI::Reverb("COPPER SPRING"); style.chassisColor = {0.6f, 0.4f, 0.3f, 1.0f}; style.textColor = {0.1f, 0.05f, 0.0f, 1.0f};
        return std::make_shared<RackUnitUI>(this, style); 
    }
private:
    float m_sr;
};

class CathedralProcessor : public FluxPluginProcessor {
public:
    CathedralProcessor(float sr) {
        m_l = std::make_unique<SimpleReverb>(sr);
        m_r = std::make_unique<SimpleReverb>(sr);
    }
    void processBlock(const float* in, float* out, int total) override {
        m_l->setParams(10.0f, 0.98f, getParam(1));
        m_r->setParams(10.0f, 0.98f, getParam(1));
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
    Cathedral(int buf, float sr) : FluxPlugin("Cathedral", buf, sr), m_sr(sr) {
        addParam("Decay", 2.0f, 20.0f, 5.0f);
        addParam("Mix", 0.0f, 1.0f, 0.5f);
    }
    std::shared_ptr<FluxProcessor> createProcessor() override { return std::make_shared<CathedralProcessor>(m_sr); }
    std::shared_ptr<Component> createEditor(const NodeEditorContext&) override { 
        auto style = RackUnitUI::Reverb("CATHEDRAL"); style.chassisColor = {0.9f, 0.9f, 0.9f, 1.0f}; style.textColor = {0.2f, 0.2f, 0.25f, 1.0f};
        return std::make_shared<RackUnitUI>(this, style); 
    }
private:
    float m_sr;
};

class GrainVerbProcessor : public FluxPluginProcessor {
public:
    GrainVerbProcessor() : m_pos(0) {
        m_buffer.assign(44100, 0.0f);
    }
    void processBlock(const float* in, float* out, int total) override {
        float mix = getParam(1);
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
    size_t m_pos;
};

class GrainVerb : public FluxPlugin {
public:
    GrainVerb(int buf, float sr) : FluxPlugin("Grain Verb", buf, sr) {
        addParam("Density", 0.0f, 1.0f, 0.5f);
        addParam("Mix", 0.0f, 1.0f, 0.3f);
    }
    std::shared_ptr<FluxProcessor> createProcessor() override { return std::make_shared<GrainVerbProcessor>(); }
    std::shared_ptr<Component> createEditor(const NodeEditorContext&) override { 
        auto style = RackUnitUI::Reverb("GRAIN VERB"); style.chassisColor = {0.1f, 0.15f, 0.1f, 1.0f}; style.textColor = {0.6f, 0.9f, 0.6f, 1.0f};
        return std::make_shared<RackUnitUI>(this, style); 
    }
};

// ============================================================================
// 4. TIME (DELAY)
// ============================================================================

class EchoPlexProcessor : public FluxPluginProcessor {
public:
    EchoPlexProcessor(float sr) : m_pos(0) {
        m_buffer.assign((size_t)(sr * 2.0f), 0.0f);
        m_wf = std::make_unique<AnalogBase::WowFlutterGenerator>(sr);
    }
    void processBlock(const float* in, float* out, int total) override {
        float fb = getParam(1);
        m_wf->setIntensity(getParam(2) * 0.01f, 0.0f);
        for (int i = 0; i < total; ++i) {
            float speedMod = m_wf->next();
            float delaySamps = getParam(0) * 44100.0f * (1.0f + speedMod); // Using fixed SR for simplicity in SDK
            size_t readPos = (m_pos + m_buffer.size() - (size_t)delaySamps) % m_buffer.size();
            float delayOut = std::tanh(m_buffer[readPos]);
            m_buffer[m_pos] = in[i] + delayOut * fb;
            out[i] = in[i] + delayOut;
            if (++m_pos >= m_buffer.size()) m_pos = 0;
        }
    }
private:
    std::vector<float> m_buffer;
    size_t m_pos;
    std::unique_ptr<AnalogBase::WowFlutterGenerator> m_wf;
};

class EchoPlex : public FluxPlugin {
public:
    EchoPlex(int buf, float sr) : FluxPlugin("Echo-Plex", buf, sr), m_sr(sr) {
        addParam("Time", 0.1f, 2.0f, 0.5f);
        addParam("Feedback", 0.0f, 0.95f, 0.4f);
        addParam("Wow", 0.0f, 1.0f, 0.2f);
    }
    std::shared_ptr<FluxProcessor> createProcessor() override { return std::make_shared<EchoPlexProcessor>(m_sr); }
    std::shared_ptr<Component> createEditor(const NodeEditorContext&) override { return std::make_shared<RackUnitUI>(this, RackUnitUI::Delay("ECHO-PLEX")); }
private:
    float m_sr;
};

class BBD_BucketProcessor : public FluxPluginProcessor {
public:
    BBD_BucketProcessor(float sr) : m_pos(0) {
        m_buffer.assign((size_t)(sr * 1.0f), 0.0f);
        m_lpf = std::make_unique<BiquadFilterNode>(FilterType::LowPass, 2000.0f, 0.7f, sr);
    }
    void processBlock(const float* in, float* out, int total) override {
        m_lpf->setCutoff(10000.0f - getParam(1) * 9000.0f);
        size_t delay = (size_t)(getParam(0) * 44100.0f);
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
    size_t m_pos;
    std::unique_ptr<BiquadFilterNode> m_lpf;
};

class BBD_Bucket : public FluxPlugin {
public:
    BBD_Bucket(int buf, float sr) : FluxPlugin("BBD-Bucket", buf, sr), m_sr(sr) {
        addParam("Time", 0.01f, 0.5f, 0.1f);
        addParam("Darkness", 0.0f, 1.0f, 0.5f);
    }
    std::shared_ptr<FluxProcessor> createProcessor() override { return std::make_shared<BBD_BucketProcessor>(m_sr); }
    std::shared_ptr<Component> createEditor(const NodeEditorContext&) override { 
        auto style = RackUnitUI::Delay("BBD-BUCKET"); style.chassisColor = {0.1f, 0.1f, 0.2f, 1.0f}; style.textColor = {0.6f, 0.8f, 1.0f, 1.0f};
        return std::make_shared<RackUnitUI>(this, style); 
    }
private:
    float m_sr;
};

class Reverse_DelayProcessor : public FluxPluginProcessor {
public:
    Reverse_DelayProcessor(float sr) : m_pos(0) {
        m_buffer.assign((size_t)sr, 0.0f);
    }
    void processBlock(const float* in, float* out, int total) override {
        float mix = getParam(0);
        for(int i=0; i<total; ++i) {
            m_buffer[m_pos] = in[i];
            size_t r = (m_buffer.size() - m_pos) % m_buffer.size();
            out[i] = in[i] * (1.0f - mix) + m_buffer[r] * mix;
            if (++m_pos >= m_buffer.size()) m_pos = 0;
        }
    }
private:
    std::vector<float> m_buffer;
    size_t m_pos;
};

class Reverse_Delay : public FluxPlugin {
public:
    Reverse_Delay(int buf, float sr) : FluxPlugin("Reverse", buf, sr), m_sr(sr) {
        addParam("Mix", 0.0f, 1.0f, 0.5f);
    }
    std::shared_ptr<FluxProcessor> createProcessor() override { return std::make_shared<Reverse_DelayProcessor>(m_sr); }
    std::shared_ptr<Component> createEditor(const NodeEditorContext&) override { return std::make_shared<RackUnitUI>(this, RackUnitUI::Delay("REVERSE")); }
private:
    float m_sr;
};

class PingPong_DelayProcessor : public FluxPluginProcessor {
public:
    PingPong_DelayProcessor(float sr) : m_pos(0) {
        m_l.assign((size_t)sr, 0.0f);
        m_r.assign((size_t)sr, 0.0f);
    }
    void processBlock(const float* in, float* out, int total) override {
        float fb = getParam(1);
        size_t delay = (size_t)(getParam(0) * 44100.0f);
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
    size_t m_pos;
};

class PingPong_Delay : public FluxPlugin {
public:
    PingPong_Delay(int buf, float sr) : FluxPlugin("Ping-Pong", buf, sr), m_sr(sr) {
        addParam("Time", 0.1f, 1.0f, 0.4f);
        addParam("Feedback", 0.0f, 0.9f, 0.5f);
    }
    std::shared_ptr<FluxProcessor> createProcessor() override { return std::make_shared<PingPong_DelayProcessor>(m_sr); }
    std::shared_ptr<Component> createEditor(const NodeEditorContext&) override { return std::make_shared<RackUnitUI>(this, RackUnitUI::Delay("PING-PONG")); }
private:
    float m_sr;
};

class SpaceShiftProcessor : public FluxPluginProcessor {
public:
    SpaceShiftProcessor() : m_pos(0), m_phase(0.0f) {
        m_buffer.assign(4000, 0.0f);
    }
    void processBlock(const float* in, float* out, int total) override {
        float width = getParam(0) * 50.0f;
        float rate = getParam(1);
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
    size_t m_pos;
    float m_phase;
};

class SpaceShift : public FluxPlugin {
public:
    SpaceShift(int buf, float sr) : FluxPlugin("Space Shift", buf, sr) {
        addParam("Width", 0.0f, 1.0f, 0.5f);
        addParam("Rate", 0.1f, 5.0f, 1.0f);
    }
    std::shared_ptr<FluxProcessor> createProcessor() override { return std::make_shared<SpaceShiftProcessor>(); }
    std::shared_ptr<Component> createEditor(const NodeEditorContext&) override { 
        auto style = RackUnitUI::Utility("SPACE SHIFT"); style.chassisColor = {0.2f, 0.0f, 0.2f, 1.0f}; style.textColor = {0.8f, 0.6f, 1.0f, 1.0f};
        return std::make_shared<RackUnitUI>(this, style); 
    }
};

// ============================================================================
// 5. LIMITER
// ============================================================================

class TubeLimiterProcessor : public FluxPluginProcessor {
public:
    TubeLimiterProcessor(std::shared_ptr<MeterSource> meterSource) : m_meterSource(meterSource) {}
    void processBlock(const float* in, float* out, int total) override {
        float thresh = std::pow(10.0f, getParam(0) / 20.0f);
        float ceiling = std::pow(10.0f, getParam(1) / 20.0f);
        float peak = 0.0f;
        for (int i = 0; i < total; ++i) {
            float absS = std::abs(in[i]);
            if (absS > peak) peak = absS;
            out[i] = ((absS > thresh) ? (std::tanh(in[i] / thresh) * thresh) : in[i]) * ceiling;
        }
        float gr = (peak > thresh) ? (peak - thresh) : 0.0f;
        if (m_meterSource) m_meterSource->updateMeter(0, gr);
        m_lastGR = gr;
    }
    float getGR() const { return m_lastGR; }
private:
    std::shared_ptr<MeterSource> m_meterSource;
    float m_lastGR = 0.0f;
};

class TubeLimiter : public FluxPlugin {
public:
    TubeLimiter(int buf, float sr) : FluxPlugin("Tube Limiter", buf, sr) {
        addParam("Threshold", -20.0f, 0.0f, 0.0f);
        addParam("Output", -10.0f, 0.0f, 0.0f);
        m_meterSource->addMeter("GR");
    }
    std::shared_ptr<FluxProcessor> createProcessor() override { 
        auto p = std::make_shared<TubeLimiterProcessor>(m_meterSource);
        m_lastProc = p.get();
        return p;
    }
    float getLatestGR() const { return m_lastProc ? m_lastProc->getGR() : 0.0f; }
    std::shared_ptr<Component> createEditor(const NodeEditorContext& ctx) override {
        auto style = RackUnitUI::Utility("LIMITER"); style.chassisColor = {0.6f, 0.2f, 0.2f, 1.0f}; style.showMeter = true;
        return std::make_shared<RackUnitUI>(this, style);
    }
private:
    TubeLimiterProcessor* m_lastProc = nullptr;
};

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
class SaturationProcessor : public FluxPluginProcessor {
public:
    void processBlock(const float* in, float* out, int total) override {
        // We cannot use getParam inside Processor easily without the Node reference in this simplified architecture.
        // However, updateParameters logic assumes we have cache.
        // For ValidFluxPluginProcessor (standard), we use updateParameters(). 
        // These inline classes are breaking the pattern.
        // Let's fix them to standard.
    }
    // Override standard update
    void updateParameters(const float* params) override { m_drive = params[0]; }
    void process(const float** inputs, float** outputs, int frames) override {
        const float* in = inputs[0]; float* out = outputs[0];
        for(int i=0; i<frames; ++i) out[i] = std::tanh(in[i] * m_drive);
    }
private:
    float m_drive = 1.0f;
};

class Saturation : public FluxPlugin {
public:
    Saturation(int buf, float sr) : FluxPlugin("Saturation", buf, sr) {
        addParam("Drive", 0.0f, 10.0f, 1.0f);
    }
    std::shared_ptr<FluxProcessor> createProcessor() override { return std::make_shared<SaturationProcessor>(); }
    std::shared_ptr<Component> createEditor(const NodeEditorContext&) override { 
        auto style = RackUnitUI::Utility("SATURATION"); style.chassisColor = {0.5f, 0.1f, 0.0f, 1.0f};
        return std::make_shared<RackUnitUI>(this, style); 
    }
};

class LookaheadLimiterProcessor : public FluxPluginProcessor {
public:
    LookaheadLimiterProcessor(float sr) : m_pos(0) {
        size_t lookahead = (size_t)(0.005f * sr);
        m_buffer.assign(lookahead * 2, 0.0f);
    }
    
    void updateParameters(const float* params) override {
        m_thresh = params[0];
        m_ceiling = params[1];
    }

    void process(const float** inputs, float** outputs, int frames) override {
         const float* in = inputs[0]; float* out = outputs[0];
         processBlock(in, out, frames * 2);
    }

    void processBlock(const float* in, float* out, int total) override {
        float thresh = std::pow(10.0f, m_thresh / 20.0f);
        float ceiling = std::pow(10.0f, m_ceiling / 20.0f);
        for (int i = 0; i < total; ++i) {
            float input = in[i];
            float delayed = m_buffer[m_pos];
            m_buffer[m_pos] = input;
            m_pos = (m_pos + 1) % m_buffer.size();
            float peak = std::abs(input); 
            float gr = (peak > thresh) ? (thresh / peak) : 1.0f;
            out[i] = delayed * gr * ceiling;
        }
    }
private:
    std::vector<float> m_buffer;
    size_t m_pos;
    float m_thresh = 0, m_ceiling = 0;
};

class LookaheadLimiter : public FluxPlugin {
public:
    LookaheadLimiter(int buf, float sr) : FluxPlugin("Lookahead Limiter", buf, sr), m_sr(sr) {
        addParam("Threshold", -20.0f, 0.0f, 0.0f);
        addParam("Ceiling", -10.0f, 0.0f, -0.1f);
        m_meterSource->addMeter("GR");
    }
    size_t getLatency() const override { return (size_t)(0.005f * m_sr); }
    std::shared_ptr<FluxProcessor> createProcessor() override { return std::make_shared<LookaheadLimiterProcessor>(m_sr); }
    std::shared_ptr<Component> createEditor(const NodeEditorContext&) override { 
        auto style = RackUnitUI::Utility("L2 LIMITER"); style.chassisColor = {0.1f, 0.1f, 0.1f, 1.0f}; style.textColor = {1.0f, 0.8f, 0.0f, 1.0f};
        return std::make_shared<RackUnitUI>(this, style); 
    }
private:
    float m_sr;
};

} // namespace Beam
#endif // ANALOG_SUITE_HPP
