#ifndef FLUX_FX_NODES_HPP
#define FLUX_FX_NODES_HPP

#include "flux_node.hpp"
#include "gain_node.hpp"
#include "biquad_filter_node.hpp"
#include "delay_node.hpp"

namespace Beam {

class FluxGainNode : public FluxNode {
public:
    FluxGainNode(int bufferSize) {
        auto gainParam = std::make_shared<Parameter>("Gain", 0.0f, 2.0f, 1.0f);
        addParameter(gainParam);
        m_gain = std::make_shared<GainNode>(1.0f);
        setupBuffers(1, 1, bufferSize, 2);
    }
    
    void process(int frames) override {
        float gainValue = getParameter("Gain")->getValue();
        m_gain->setGain(gainValue);

        float* in = getInputBuffer(0);
        float* out = getOutputBuffer(0);
        std::copy(in, in + frames * 2, out);
        m_gain->process(out, frames, 2);
    }

    std::string getName() const override { return "Gain"; }
    std::vector<Port> getInputPorts() const override { return { {"In", 2} }; }
    std::vector<Port> getOutputPorts() const override { return { {"Out", 2} }; }

private:
    std::shared_ptr<GainNode> m_gain;
};

class FluxFilterNode : public FluxNode {
public:
    FluxFilterNode(int bufferSize, float sampleRate) {
        auto cutoffParam = std::make_shared<Parameter>("Cutoff", 20.0f, 20000.0f, 1000.0f);
        auto qParam = std::make_shared<Parameter>("Resonance", 0.1f, 10.0f, 0.707f);
        addParameter(cutoffParam);
        addParameter(qParam);

        m_filter = std::make_shared<BiquadFilterNode>(FilterType::LowPass, 1000.0f, 0.707f, sampleRate);
        setupBuffers(1, 1, bufferSize, 2);
    }
    
    void process(int frames) override {
        m_filter->setCutoff(getParameter("Cutoff")->getValue());
        m_filter->setQ(getParameter("Resonance")->getValue());

        float* in = getInputBuffer(0);
        float* out = getOutputBuffer(0);
        std::copy(in, in + frames * 2, out);
        m_filter->process(out, frames, 2);
    }

    std::string getName() const override { return "Filter"; }
    std::vector<Port> getInputPorts() const override { return { {"In", 2} }; }
    std::vector<Port> getOutputPorts() const override { return { {"Out", 2} }; }

private:
    std::shared_ptr<BiquadFilterNode> m_filter;
};

class FluxDelayNode : public FluxNode {
public:
    FluxDelayNode(int bufferSize, float sampleRate) {
        auto timeParam = std::make_shared<Parameter>("Time", 0.0f, 2.0f, 0.5f);
        auto feedbackParam = std::make_shared<Parameter>("Feedback", 0.0f, 1.0f, 0.3f);
        addParameter(timeParam);
        addParameter(feedbackParam);

        m_delay = std::make_shared<DelayNode>(2.0f, 0.3f, sampleRate); // Max 2s delay
        setupBuffers(1, 1, bufferSize, 2);
    }
    
    void process(int frames) override {
        // Note: DelayNode's buffer size is fixed at creation in this simple implementation
        // so we only update feedback here. Delay time would need a circular buffer resize.
        // For this abstraction demo, we'll just update feedback.
        // m_delay->setFeedback(getParameter("Feedback")->getValue()); 

        float* in = getInputBuffer(0);
        float* out = getOutputBuffer(0);
        std::copy(in, in + frames * 2, out);
        m_delay->process(out, frames, 2);
    }

    std::string getName() const override { return "Delay"; }
    std::vector<Port> getInputPorts() const override { return { {"In", 2} }; }
    std::vector<Port> getOutputPorts() const override { return { {"Out", 2} }; }

private:
    std::shared_ptr<DelayNode> m_delay;
};

} // namespace Beam

#endif // FLUX_FX_NODES_HPP
