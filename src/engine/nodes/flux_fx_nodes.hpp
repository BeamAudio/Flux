#ifndef FLUX_FX_NODES_HPP
#define FLUX_FX_NODES_HPP

#include "engine/plugins/flux_plugin.hpp"
#include "engine/nodes/biquad_filter_node.hpp"
#include "engine/nodes/delay_node.hpp"
#include "interface/editors/filter_editor.hpp"

namespace Beam {

// --- Gain Processor ---
class FluxGainProcessor : public FluxPluginProcessor {
public:
    void processBlock(const float* input, float* output, int totalSamples) override {
        float gain = getParam(0);
        for (int i = 0; i < totalSamples; ++i) {
            output[i] = input[i] * gain;
        }
    }
};

class FluxGainNode : public FluxPlugin {
public:
    FluxGainNode(int bufferSize) : FluxPlugin("Gain", bufferSize, 44100.0f) {
        addParam("Gain", 0.0f, 2.0f, 1.0f);
    }
    std::unique_ptr<FluxProcessor> createProcessor() override {
        return std::make_unique<FluxGainProcessor>();
    }
};

// --- Filter Processor ---
class FluxFilterProcessor : public FluxPluginProcessor {
public:
    FluxFilterProcessor(float sampleRate) : m_sampleRate(sampleRate) {
        m_filter = std::make_unique<BiquadFilterNode>(FilterType::LowPass, 1000.0f, 0.707f, sampleRate);
    }
    void processBlock(const float* input, float* output, int totalSamples) override {
        m_filter->setCutoff(getParam(0));
        m_filter->setQ(getParam(1));
        std::copy(input, input + totalSamples, output);
        m_filter->process(output, totalSamples / 2, 2);
    }
    BiquadFilterNode* getInternalFilter() { return m_filter.get(); }
private:
    float m_sampleRate;
    std::unique_ptr<BiquadFilterNode> m_filter;
};

class FluxFilterNode : public FluxPlugin {
public:
    FluxFilterNode(int bufferSize, float sampleRate) 
        : FluxPlugin("Filter", bufferSize, sampleRate), m_sampleRate(sampleRate) 
    {
        addParam("Cutoff", 20.0f, 20000.0f, 1000.0f);
        addParam("Reso", 0.1f, 10.0f, 0.707f);
    }
    std::unique_ptr<FluxProcessor> createProcessor() override {
        auto proc = std::make_unique<FluxFilterProcessor>(m_sampleRate);
        m_lastProcessor = proc.get();
        return proc;
    }
    BiquadFilterNode* getInternalFilter() { 
        if (m_lastProcessor) return m_lastProcessor->getInternalFilter();
        if (!m_dummyFilter) m_dummyFilter = std::make_unique<BiquadFilterNode>(FilterType::LowPass, 1000.0f, 0.707f, m_sampleRate);
        return m_dummyFilter.get();
    }
    std::shared_ptr<Component> createEditor(const NodeEditorContext& ctx) override {
        return std::make_shared<FilterEditor>(this);
    }
private:
    float m_sampleRate;
    FluxFilterProcessor* m_lastProcessor = nullptr;
    std::unique_ptr<BiquadFilterNode> m_dummyFilter;
};

// --- Delay Processor ---
class FluxDelayProcessor : public FluxPluginProcessor {
public:
    FluxDelayProcessor(float sampleRate) {
        m_delay = std::make_unique<DelayNode>(2.0f, 0.3f, sampleRate);
    }
    void processBlock(const float* input, float* output, int totalSamples) override {
        m_delay->setDelayTime(getParam(0));
        m_delay->setFeedback(getParam(1));
        std::copy(input, input + totalSamples, output);
        m_delay->process(output, totalSamples / 2, 2);
    }
private:
    std::unique_ptr<DelayNode> m_delay;
};

class FluxDelayNode : public FluxPlugin {
public:
    FluxDelayNode(int bufferSize, float sampleRate) 
        : FluxPlugin("Delay", bufferSize, sampleRate), m_sampleRate(sampleRate) 
    {
        addParam("Time", 0.0f, 2.0f, 0.5f);
        addParam("Feedback", 0.0f, 0.95f, 0.3f);
    }
    std::unique_ptr<FluxProcessor> createProcessor() override {
        return std::make_unique<FluxDelayProcessor>(m_sampleRate);
    }
private:
    float m_sampleRate;
};

} // namespace Beam

#endif // FLUX_FX_NODES_HPP
