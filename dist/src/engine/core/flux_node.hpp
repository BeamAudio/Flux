#ifndef FLUX_NODE_HPP
#define FLUX_NODE_HPP

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <map>
#include "engine/dsp/audio_buffer.hpp"
#include "engine/session/parameter.hpp"
#include "engine/midi/midi_event.hpp"
#include "engine/dsp/meter_source.hpp"
#include "engine/core/flux_processor.hpp"
#include "json.hpp"

namespace Beam {

class Component;
class AudioDeviceManager;

struct NodeEditorContext {
    AudioDeviceManager* deviceManager = nullptr;
};

/**
 * @class FluxNode
 * @brief The high-level model representing a DSP entity in the DAW.
 *
 * FluxNode is the persistent, UI-facing part of a processor. It lives on the
 * main thread and handles state management, serialization, and UI generation.
 * It acts as a factory for its real-time counterpart, the FluxProcessor.
 */
class FluxNode {
public:
    virtual ~FluxNode() = default;

    /**
     * @brief Safe deallocation across DLL boundaries.
     */
    virtual void releaseNode() { delete this; }

    virtual std::string getName() const = 0;

    /**
     * @brief Creates a custom UI editor for this node.
     */
    virtual std::shared_ptr<Component> createEditor(const NodeEditorContext& context);

    /**
     * @struct Port
     */
    struct Port {
        std::string name;
        int channels;
        enum Type { Audio, Sidechain } type = Audio;
    };

    virtual std::vector<Port> getInputPorts() const { return {}; }
    virtual std::vector<Port> getOutputPorts() const { return {}; }
    virtual size_t getLatency() const { return 0; }

    /**
     * @brief Factory method to create the real-time processor for this node.
     * Called during graph compilation.
     */
    virtual std::shared_ptr<FluxProcessor> createProcessor() = 0;

    virtual void onTransportStateChanged(bool playing) {}
    virtual void onTransportSeek(size_t frame) {}
    
    // Optional hook for visual updates (meters, displays)
    virtual void updateVisuals() {}

    // Serialization
    virtual nlohmann::json serialize() const {
        nlohmann::json data;
        data["name"] = getName();
        data["bypassed"] = m_bypassed.load();
        
        nlohmann::json params;
        for (const auto& [name, p] : m_parameters) {
            params[name] = p->getValue();
        }
        data["parameters"] = params;
        return data;
    }

    virtual void deserialize(const nlohmann::json& data) {
        if (data.contains("bypassed")) m_bypassed = data["bypassed"];
        if (data.contains("parameters")) {
            for (auto& [key, val] : data["parameters"].items()) {
                if (auto p = getParameter(key)) {
                    p->setValue(val);
                }
            }
        }
    }

    // Parameters
    void addParameter(std::shared_ptr<Parameter> p) { 
        m_parameters[p->getName()] = p; 
        m_parameterOrder.push_back(p);
    }
    std::shared_ptr<Parameter> getParameter(const std::string& name) { 
        auto it = m_parameters.find(name);
        return (it != m_parameters.end()) ? it->second : nullptr; 
    }
    const std::map<std::string, std::shared_ptr<Parameter>>& getParameters() const { return m_parameters; }
    const std::vector<std::shared_ptr<Parameter>>& getParameterOrder() const { return m_parameterOrder; }

    void setBypassed(bool bypass) { m_bypassed = bypass; }
    bool isBypassed() const { return m_bypassed; }

    std::shared_ptr<MeterSource> getMeterSource() { return m_meterSource; }

protected:
    std::map<std::string, std::shared_ptr<Parameter>> m_parameters;
    std::vector<std::shared_ptr<Parameter>> m_parameterOrder;
    std::shared_ptr<MeterSource> m_meterSource = std::make_shared<MeterSource>();
    std::atomic<bool> m_bypassed{false};
};

} // namespace Beam

#endif // FLUX_NODE_HPP





