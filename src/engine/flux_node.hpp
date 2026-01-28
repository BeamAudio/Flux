#ifndef FLUX_NODE_HPP
#define FLUX_NODE_HPP

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <map>
#include "../session/parameter.hpp"
#include "midi_event.hpp"

namespace Beam {

/**
 * @class FluxNode
 * @brief Base class for all audio and MIDI processing nodes in the Flux Graph.
 */
class FluxNode {
public:
    virtual ~FluxNode() = default;

    /**
     * @struct Port
     * @brief Describes an audio input or output port.
     */
    struct Port {
        std::string name;
        int channels;
    };

    /**
     * @brief Main audio processing method. Must be implemented by subclasses.
     * @param frames Number of frames to process in the current block.
     */
    virtual void process(int frames) = 0;

    /**
     * @brief Optional MIDI processing. Called before process() in the engine loop.
     */
    virtual void processMIDI(const MIDIBuffer& midi) {}

    /**
     * @brief Responds to global transport changes (Play/Pause).
     */
    virtual void onTransportStateChanged(bool playing) {}

    /**
     * @brief Responds to timeline seeking.
     */
    virtual void onTransportSeek(size_t frame) { m_currentFrame = frame; }

    /**
     * @brief Sets the current playhead position for this block.
     */
    void setCurrentFrame(size_t frame) { m_currentFrame = frame; }

    virtual std::string getName() const = 0;
    virtual std::vector<Port> getInputPorts() const = 0;
    virtual std::vector<Port> getOutputPorts() const = 0;

    float* getInputBuffer(int portIdx) { return m_inputs[portIdx].data(); }
    float* getOutputBuffer(int portIdx) { return m_outputs[portIdx].data(); }

    void setBypass(bool bypass) { m_bypassed = bypass; }
    bool isBypassed() const { return m_bypassed; }

    void addParameter(std::shared_ptr<Parameter> param) {
        m_parameters[param->getName()] = param;
    }

    std::shared_ptr<Parameter> getParameter(const std::string& name) {
        auto it = m_parameters.find(name);
        return (it != m_parameters.end()) ? it->second : nullptr;
    }

    const std::map<std::string, std::shared_ptr<Parameter>>& getParameters() const {
        return m_parameters;
    }

protected:
    /**
     * @brief Pre-allocates buffers for inputs and outputs.
     */
    void setupBuffers(int numInputs, int numOutputs, int bufferSize, int channels) {
        m_inputs.assign(numInputs, std::vector<float>(bufferSize * channels, 0.0f));
        m_outputs.assign(numOutputs, std::vector<float>(bufferSize * channels, 0.0f));
    }

    std::vector<std::vector<float>> m_inputs;
    std::vector<std::vector<float>> m_outputs;
    std::map<std::string, std::shared_ptr<Parameter>> m_parameters;
    std::atomic<bool> m_bypassed{false};
    size_t m_currentFrame = 0;
};

} // namespace Beam

#endif // FLUX_NODE_HPP
