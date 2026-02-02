#ifndef FLUX_PLUGIN_HPP
#define FLUX_PLUGIN_HPP

#include "engine/core/flux_node.hpp"
#include <string>

namespace Beam {

/**
 * @class FluxPluginProcessor
 * @brief The real-time worker for FluxPlugins.
 */
class FluxPluginProcessor : public FluxProcessor {
public:
    virtual ~FluxPluginProcessor() = default;

    // SDK process interface (mirroring old FluxPlugin for compatibility)
    virtual void processBlock(const float* input, float* output, int totalSamples) = 0;

    void process(const float** inputs, float** outputs, int frames) override {
        // Collect parameter values (handled by base FluxProcessor updateParameters)
        // Default behavior: Stereo interleaved processing for simple SDK plugins
        processBlock(inputs[0], outputs[0], frames * 2);
    }
    
    void updateParameters(const float* params) override {
        m_paramValues = params;
    }

protected:
    float getParam(int index) const { return m_paramValues ? m_paramValues[index] : 0.0f; }
    const float* m_paramValues = nullptr;
};

/**
 * Beam Flux SDK: FluxPlugin
 * A high-level abstraction for creating custom DSP effects.
 */
class FluxPlugin : public FluxNode {
public:
    FluxPlugin(const std::string& name, int bufferSize, float sampleRate) 
        : m_pluginName(name), m_sampleRate(sampleRate) {}

    virtual ~FluxPlugin() = default;

    std::string getName() const override { return m_pluginName; }
    std::vector<Port> getInputPorts() const override { return { {"In", 2} }; }
    std::vector<Port> getOutputPorts() const override { return { {"Out", 2} }; }

protected:
    void addParam(const std::string& name, float min, float max, float initial) {
        addParameter(std::make_shared<Parameter>(name, min, max, initial));
    }

    float getSampleRate() const { return m_sampleRate; }

private:
    std::string m_pluginName;
    float m_sampleRate;
};

} // namespace Beam

#endif // FLUX_PLUGIN_HPP






