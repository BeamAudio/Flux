#ifndef FLUX_PLUGIN_HPP
#define FLUX_PLUGIN_HPP

#include "flux_node.hpp"
#include <string>

namespace Beam {

/**
 * Beam Flux SDK: FluxPlugin
 * A high-level abstraction for creating custom DSP effects.
 * Inherit from this class to design your own filters and processors.
 */
class FluxPlugin : public FluxNode {
public:
    FluxPlugin(const std::string& name, int bufferSize, float sampleRate) 
        : m_pluginName(name), m_sampleRate(sampleRate) 
    {
        // Default to 1 Stereo Input and 1 Stereo Output
        setupBuffers(1, 1, bufferSize, 2);
    }

    // --- SDK PROCESS INTERFACE ---
    // Implement this to write your DSP logic.
    // 'input' and 'output' are interleaved stereo buffers (L, R, L, R...)
    virtual void processBlock(const float* input, float* output, int totalSamples) = 0;

    /**
     * @brief Handle MIDI events in your plugin.
     * @param midi The buffer containing all MIDI events for the current block.
     */
    virtual void processEvents(const MIDIBuffer& midi) {}

    // Implementation of the low-level FluxNode interface
    void processMIDI(const MIDIBuffer& midi) override {
        processEvents(midi);
    }

    void process(int frames) override {
        // Automatically handle bypass logic at the SDK level
        if (isBypassed()) {
            float* in = getInputBuffer(0);
            float* out = getOutputBuffer(0);
            std::copy(in, in + frames * 2, out);
            return;
        }
        processBlock(getInputBuffer(0), getOutputBuffer(0), frames * 2);
    }

    std::string getName() const override { return m_pluginName; }
    std::vector<Port> getInputPorts() const override { return { {"In", 2} }; }
    std::vector<Port> getOutputPorts() const override { return { {"Out", 2} }; }


protected:
    // --- SDK UTILITIES ---
    
    // Register a parameter that will automatically appear in the GUI
    void addParam(const std::string& name, float min, float max, float initial) {
        addParameter(std::make_shared<Parameter>(name, min, max, initial));
    }

    // Retrieve current parameter value (thread-safe)
    float getParam(const std::string& name) {
        auto p = getParameter(name);
        return p ? p->getValue() : 0.0f;
    }

    float getSampleRate() const { return m_sampleRate; }

private:
    std::string m_pluginName;
    float m_sampleRate;
};

} // namespace Beam

#endif // FLUX_PLUGIN_HPP






