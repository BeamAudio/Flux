#ifndef FLUX_FX_NODES_HPP
#define FLUX_FX_NODES_HPP

#include "sdk/beam_sdk.hpp"
#include "engine/nodes/biquad_filter_node.hpp"
#include "engine/nodes/delay_node.hpp"
#include "interface/editors/filter_editor.hpp"

namespace Beam {

// --- Gain Node ---
class FluxGainNode : public SDK::BeamPlugin {
public:
    FluxGainNode(int, float) : BeamPlugin("Gain", "Utility") {
        gain = &addFloatParam("Gain", 0.0f, 2.0f, 1.0f);
    }
    
    void process(float** io, int frames) override {
        float g = gain->getValue();
        for(int ch=0; ch<2; ++ch) {
            if(!io[ch]) continue;
            for (int i = 0; i < frames; ++i) {
                io[ch][i] *= g;
            }
        }
    }
private:
    Parameter* gain;
};

// --- Filter Node ---
class FluxFilterNode : public SDK::BeamPlugin {
public:
    FluxFilterNode(int, float sampleRate) 
        : BeamPlugin("Filter", "Filter"), m_sampleRate(sampleRate) 
    {
        cutoff = &addFloatParam("Cutoff", 20.0f, 20000.0f, 1000.0f);
        reso = &addFloatParam("Reso", 0.1f, 10.0f, 0.707f);
        for(int i=0; i<2; ++i) m_filter[i] = std::make_unique<BiquadFilterNode>(FilterType::LowPass, 1000.0f, 0.707f, sampleRate);
    }

    void process(float** io, int frames) override {
        float cf = cutoff->getValue();
        float q = reso->getValue();
        for(int ch=0; ch<2; ++ch) {
            if(!io[ch]) continue;
            m_filter[ch]->setCutoff(cf);
            m_filter[ch]->setQ(q);
            m_filter[ch]->process(io[ch], frames, 1);
        }
    }

    void prepareToPlay(float sr, int) override {
        m_sampleRate = sr;
        for(int i=0; i<2; ++i) m_filter[i] = std::make_unique<BiquadFilterNode>(FilterType::LowPass, 1000.0f, 0.707f, sr);
    }

    BiquadFilterNode* getInternalFilter(int ch = 0) { return m_filter[ch].get(); }
    
    std::shared_ptr<Component> createEditor(const NodeEditorContext& ctx) override {
        return std::make_shared<FilterEditor>(this);
    }

private:
    float m_sampleRate;
    Parameter *cutoff, *reso;
    std::unique_ptr<BiquadFilterNode> m_filter[2];
};

// --- Delay Node ---
class FluxDelayNode : public SDK::BeamPlugin {
public:
    FluxDelayNode(int, float sampleRate) 
        : BeamPlugin("Delay", "Delay"), m_sampleRate(sampleRate) 
    {
        time = &addFloatParam("Time", 0.0f, 2.0f, 0.5f);
        feedback = &addFloatParam("Feedback", 0.0f, 0.95f, 0.3f);
        for(int i=0; i<2; ++i) m_delay[i] = std::make_unique<DelayNode>(2.0f, 0.3f, sampleRate);
    }

    void process(float** io, int frames) override {
        float t = time->getValue();
        float fb = feedback->getValue();
        for(int ch=0; ch<2; ++ch) {
            if(!io[ch]) continue;
            m_delay[ch]->setDelayTime(t);
            m_delay[ch]->setFeedback(fb);
            m_delay[ch]->process(io[ch], frames, 1);
        }
    }

    void prepareToPlay(float sr, int) override {
        m_sampleRate = sr;
        for(int i=0; i<2; ++i) m_delay[i] = std::make_unique<DelayNode>(2.0f, 0.3f, sr);
    }

private:
    float m_sampleRate;
    Parameter *time, *feedback;
    std::unique_ptr<DelayNode> m_delay[2];
};

// Registration
REGISTER_BEAM_PLUGIN(FluxGainNode)
REGISTER_BEAM_PLUGIN(FluxFilterNode)
REGISTER_BEAM_PLUGIN(FluxDelayNode)

} // namespace Beam

#endif // FLUX_FX_NODES_HPP
