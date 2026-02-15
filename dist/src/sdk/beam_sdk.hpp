/**
 * @file beam_sdk.hpp
 * @brief Single-include SDK header for Beam Audio Flux plugin development
 * 
 * Usage:
 * @code
 * #include <beam_sdk.hpp>
 * 
 * BEAM_PLUGIN(MyGain, "Gain", "Utilities") {
 *     PARAM(gain, 0.0f, 2.0f, 1.0f);
 *     
 *     void process(float** io, int frames) override {
 *         for (int i = 0; i < frames; i++) {
 *             io[0][i] *= gain();
 *             io[1][i] *= gain();
 *         }
 *     }
 * };
 * @endcode
 */

#ifndef BEAM_SDK_HPP
#define BEAM_SDK_HPP

// Core engine includes
#include "engine/core/flux_node.hpp"
#include "engine/core/flux_processor.hpp"
#include "engine/session/parameter.hpp"
#include "engine/dsp/audio_buffer.hpp"
#include "engine/plugins/plugin_registry.hpp"
#include "interface/core/component.hpp"
#include "interface/render/quad_batcher.hpp"

// SDK modules
#include "sdk/beam_widgets.hpp"

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <atomic>

namespace Beam {
namespace SDK {

// Convenient alias for plugin developers
using AudioBuffer = Beam::AudioBuffer<float>;

// =============================================================================
// BASE PLUGIN CLASS - Simplified interface for plugin developers
// =============================================================================

/**
 * @class BeamPlugin
 * @brief Base class for all SDK-based plugins. Handles boilerplate automatically.
 */
class BeamPlugin : public FluxNode {
public:
    BeamPlugin(const std::string& name, const std::string& category)
        : m_pluginName(name), m_pluginCategory(category) {
    }
    
    virtual ~BeamPlugin() = default;
    
    std::string getName() const override { return m_pluginName; }
    const std::string& getCategory() const { return m_pluginCategory; }
    const Beam::PluginMetadata& getMetadata() const { return m_metadata; }
    
    // Metadata setters for use in constructor
    void setAuthor(const std::string& author) { m_metadata.author = author; }
    void setDescription(const std::string& desc) { m_metadata.description = desc; }
    void setVersion(const std::string& version) { m_metadata.version = version; }

    Beam::PluginMetadata m_metadata;
    SDK::PanelStyle m_panelStyle;
    
    void setPanelStyle(const SDK::PanelStyle& style) { m_panelStyle = style; }
    const SDK::PanelStyle& getPanelStyle() const { return m_panelStyle; }
    
    // Default implementation for custom painting (can be overridden)
    
    // =========================================================================
    // PARAMETER HELPERS - Use these in your constructor
    // =========================================================================
    
    Parameter& addFloatParam(const std::string& name, float min, float max, float def) {
        auto p = std::make_shared<Parameter>(name, min, max, def);
        addParameter(p);
        return *p;
    }
    
    Parameter& addLogParam(const std::string& name, float min, float max, float def) {
        auto p = std::make_shared<Parameter>(name, min, max, def, MappingType::Logarithmic);
        addParameter(p);
        return *p;
    }
    
    Parameter& addBoolParam(const std::string& name, bool def) {
        auto p = std::make_shared<Parameter>(name, 0.0f, 1.0f, def ? 1.0f : 0.0f);
        addParameter(p);
        return *p;
    }
    
    // =========================================================================
    // AUDIO PROCESSING - Override this
    // =========================================================================
    
    /**
     * @brief Main audio processing callback.
     * @param io Main interleaved stereo bus
     * @param frames Number of samples to process
     */
    virtual void process(float** io, int frames) = 0;

    /**
     * @brief Overloaded processing callback with support for sidechain input.
     */
    virtual void process(float** io, float** sidechain, int frames) {
        process(io, frames);
    }
    
    void useSidechain(bool enable = true) { m_useSidechain = enable; }
    
    // =========================================================================
    // METERING & VISUALS
    // =========================================================================

    /**
     * @brief Adds a meter to the plugin.
     * @param name Name of the meter (e.g., "GR", "Level")
     * @return Index of the created meter
     */
    int addMeter(const std::string& name) {
        return m_meterSource->addMeter(name);
    }

    /**
     * @brief Updates a meter value (thread-safe, call from process).
     */
    void updateMeter(int index, float value) {
        m_meterSource->updateMeter(index, value);
    }

    /**
     * @brief Gets a meter value (thread-safe, call from UI/updateVisuals).
     */
    float getMeterValue(int index) const {
        return m_meterSource->getValue(index);
    }

    /**
     * @brief Override to update visual state based on internal metrics.
     * Called on the UI thread.
     */
    virtual void updateVisuals() {}

    // =========================================================================
    // OPTIONAL OVERRIDES & UI
    // =========================================================================
    
    /**
     * @brief Override to provide custom UI graphics.
     */
    virtual void paintCustomUI(QuadBatcher& g, float w, float h) {}
    
    /**
     * @brief Called when the sample rate or block size changes.
     */
    virtual void prepareToPlay(float sampleRate, int blockSize) {}
    
    /**
     * @brief Called on transport stop/seek to reset state.
     */
    virtual void resetState() {}
    
    // =========================================================================
    // INTERNAL - FluxNode implementation
    // =========================================================================
    
    std::vector<Port> getInputPorts() const override {
        std::vector<Port> ports = { {"Input", 2, Port::Audio} };
        if (m_useSidechain) ports.push_back({"Sidechain", 2, Port::Sidechain});
        return ports;
    }
    
    std::vector<Port> getOutputPorts() const override {
        return { {"Output", 2, Port::Audio} };
    }
    
    std::shared_ptr<FluxProcessor> createProcessor() override;
    std::shared_ptr<Component> createEditor(const NodeEditorContext& context) override;
    
    // =========================================================================
    // SERIALIZATION
    // =========================================================================

    nlohmann::json serialize() const override {
        nlohmann::json data = FluxNode::serialize();
        data["sdk_version"] = "2.0";
        data["category"] = m_pluginCategory;
        data["author"] = m_metadata.author;
        data["version"] = m_metadata.version;
        return data;
    }

    void deserialize(const nlohmann::json& data) override {
        FluxNode::deserialize(data);
    }
    
protected:
    std::string m_pluginName;
    std::string m_pluginCategory;
    float m_sampleRate = 44100.0f;
    int m_blockSize = 512;
    bool m_useSidechain = false;
};

// =============================================================================
// AUTO-GENERATED PROCESSOR - Wraps the plugin's process() method
// =============================================================================

class BeamPluginProcessor : public FluxProcessor {
public:
    BeamPluginProcessor(BeamPlugin* plugin) : m_plugin(plugin) {}
    
    void process(const float** inputs, float** outputs, int frames) override {
        if (!m_plugin || isBypassed()) {
            // Bypass: copy input 0/1 (main) to output 0/1
            for (int ch = 0; ch < 2; ch++) {
                if (inputs[ch] && outputs[ch]) {
                    std::memcpy(outputs[ch], inputs[ch], frames * sizeof(float));
                }
            }
            return;
        }
        
        // Main I/O setup
        float* mainIO[2] = { nullptr, nullptr };
        for (int ch = 0; ch < 2; ch++) {
            if (inputs[ch] && outputs[ch]) {
                std::memcpy(outputs[ch], inputs[ch], frames * sizeof(float));
                mainIO[ch] = outputs[ch];
            }
        }

        // Sidechain setup (inputs[2/3] if sidechain enabled)
        float* scPtrs[2] = { nullptr, nullptr };
        if (inputs[2]) scPtrs[0] = const_cast<float*>(inputs[2]);
        if (inputs[3]) scPtrs[1] = const_cast<float*>(inputs[3]);
        
        // Call plugin's process with sidechain support
        m_plugin->process(mainIO, scPtrs, frames);
    }
    
    void prepare(float sampleRate, int maxBlockSize) override {
        if (m_plugin) m_plugin->prepareToPlay(sampleRate, maxBlockSize);
    }
    
    void reset() override {
        if (m_plugin) m_plugin->resetState();
    }
    
private:
    BeamPlugin* m_plugin;
};

// Implement createProcessor
inline std::shared_ptr<FluxProcessor> BeamPlugin::createProcessor() {
    return std::make_shared<BeamPluginProcessor>(this);
}

// =============================================================================
// AUTO-GENERATED UI EDITOR - Renders parameter knobs automatically
// =============================================================================

class BeamPluginEditor : public Component {
public:
    BeamPluginEditor(BeamPlugin* plugin) : m_plugin(plugin) {
        setName("SDK Plugin Editor");
    }
    
    void update(float dt) override {
        Component::update(dt);
        if (m_plugin) m_plugin->updateVisuals();
    }
    
    void getPreferredSize(float& w, float& h) const override {
        // Calculate based on parameter count
        int paramCount = (int)m_plugin->getParameters().size();
        int cols = (std::max)(1, (std::min)(paramCount, 4));
        int rows = (paramCount + cols - 1) / cols;
        w = cols * 70 + 20;
        h = rows * 90 + 40;
    }
    
    void paint(QuadBatcher& batcher) override {
        const auto& style = m_plugin->getPanelStyle();
        
        // Draw Main Panel (Chassis, Screws, Title)
        drawPanel(batcher, 0, 0, m_bounds.w, m_bounds.h, style);
        
        // Let plugin paint custom UI layer
        m_plugin->paintCustomUI(batcher, m_bounds.w, m_bounds.h);
        
        // If no custom paint, draw auto-generated knobs
        const auto& params = m_plugin->getParameterOrder();
        if (params.empty()) return;
        
        float x = 25, y = 50;
        float knobSize = 50;
        float spacing = 80;
        int col = 0;
        
        for (auto& p : params) {
            if (p) {
                drawKnob(batcher, x, y, knobSize, *p, p->getName().c_str());
            }
            x += spacing;
            col++;
            if (col >= 4) {
                col = 0;
                x = 25;
                y += 100;
            }
        }

        // Draw Meter if plugin has any
        if (m_plugin->getMeterSource()->getNumMeters() > 0) {
            float val = m_plugin->getMeterSource()->getValue(0);
            drawVUMeter(batcher, m_bounds.w - 20, 50, 10, m_bounds.h - 70, val, -1.0f, true);
        }
    }
    
private:
    BeamPlugin* m_plugin;
};

// Implement createEditor
inline std::shared_ptr<Component> BeamPlugin::createEditor(const NodeEditorContext& context) {
    return std::make_shared<BeamPluginEditor>(this);
}

} // namespace SDK
} // namespace Beam

// =============================================================================
// MACROS FOR DECLARATIVE PLUGIN DEFINITION
// =============================================================================

/**
 * @brief Declares a new plugin class with automatic registration.
 * @param ClassName The C++ class name
 * @param DisplayName The name shown in the UI
 * @param Category The category in the plugin browser
 */
#define BEAM_PLUGIN(ClassName, DisplayName, Category) \
    class ClassName : public Beam::SDK::BeamPlugin { \
    public: \
        ClassName(int = 0, float sr = 44100.0f) : BeamPlugin(DisplayName, Category) { m_sampleRate = sr; initParams(); } \
        void initParams(); \
        void process(float** io, int frames) override; \
    }; \
    void ClassName::initParams()

/**
 * @brief Declares a float parameter with automatic smoothing.
 * @param name Parameter variable name
 * @param min Minimum value
 * @param max Maximum value  
 * @param def Default value
 * 
 * Access in process() with: name() or name.getNextValue()
 */
#define PARAM(name, minVal, maxVal, defVal) \
    Beam::Parameter& name = addFloatParam(#name, minVal, maxVal, defVal)

/**
 * @brief Declares a logarithmic parameter (for frequencies).
 */
#define PARAM_LOG(name, minVal, maxVal, defVal) \
    Beam::Parameter& name = addLogParam(#name, minVal, maxVal, defVal)

/**
 * @brief Declares a boolean parameter (0/1).
 */
#define PARAM_BOOL(name, defVal) \
    Beam::Parameter& name = addBoolParam(#name, defVal)

#endif // BEAM_SDK_HPP
