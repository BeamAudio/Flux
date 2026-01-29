#ifndef AUDIO_MODULE_HPP
#define AUDIO_MODULE_HPP

#include "component.hpp"
#include "port.hpp"
#include "meter.hpp"
#include "slider_modular.hpp"
#include "../engine/flux_node.hpp"
#include "../engine/input_node.hpp"
#include "../utilities/flux_audio_utils.hpp"
#include <string>
#include <vector>
#include <functional>

namespace Beam {

/**
 * @class AudioModule
 * @brief Base UI component for any FluxNode (FX, Instrument, Input, etc.)
 * Features dynamic layout and auto-sizing to prevent overlapping controls.
 */
class AudioModule : public Component {
public:
    AudioModule(std::shared_ptr<FluxNode> node, size_t nodeId, float x, float y) 
        : m_node(node), m_nodeId(nodeId), m_name(node->getName()) {
        
        if (node->getInputPorts().size() > 0)
            m_inputPort = std::make_shared<Port>(PortType::Input, this);
            
        if (node->getOutputPorts().size() > 0)
            m_outputPort = std::make_shared<Port>(PortType::Output, this);
        
        setDraggable(true);
        autoGenerateUI();
        updateLayout();
        
        // Initial positioning
        setBounds(x, y, m_bounds.w, m_bounds.h);
    }

    size_t getNodeId() const { return m_nodeId; }

    /**
     * @brief Populates the module with controls based on the node's parameters.
     */
    virtual void autoGenerateUI() {
        if (!m_node) return;
        m_children.clear();

        // 1. Special case for Audio Input (Meter)
        if (m_name == "Audio Input") {
            auto meter = std::make_shared<LuminousMeter>(LuminousMeter::Orientation::Horizontal);
            meter->setName("LevelMeter");
            m_children.push_back(meter);
        }

        // 2. Add Sliders for all parameters
        for (auto const& [name, param] : m_node->getParameters()) {
            if (name == "Master Gain" || name == "Capture Mode") continue;
            
            auto slider = std::make_shared<ModularSlider>(name, ModularSlider::Style::Horizontal);
            slider->bindParameter(param);
            m_children.push_back(slider);
        }
    }

    /**
     * @brief Calculates dimensions and positions of internal elements.
     */
    void updateLayout() {
        float curY = 40.0f; // Header padding
        float width = 160.0f;
        
        // Ensure width is sufficient for name
        float tw = AudioUtils::calculateTextWidth(m_name, 12.0f);
        width = (std::max)(width, tw + 60.0f);

        for (auto& child : m_children) {
            float h = 30.0f;
            if (child->getName() == "LevelMeter") h = 15.0f;
            
            child->setBounds(m_bounds.x + 10, m_bounds.y + curY, width - 20, h);
            curY += h + 10.0f;
        }

        m_bounds.w = width;
        m_bounds.h = (std::max)(100.0f, curY + 10.0f);
    }

    void setBounds(float x, float y, float w, float h) override {
        float dx = x - m_bounds.x;
        float dy = y - m_bounds.y;
        
        Component::setBounds(x, y, w, h);
        
        if (m_inputPort) m_inputPort->setBounds(x - 6, y + 50, 12, 12);
        if (m_outputPort) m_outputPort->setBounds(x + w - 6, y + 50, 12, 12);
        
        m_deleteBtnBounds = {x + w - 20, y + 5, 15, 15};
        
        // Position children relative to the new top-left
        float curY = 40.0f;
        for(auto& child : m_children) {
            Rect b = child->getBounds();
            child->setBounds(x + 10, y + curY, w - 20, b.h);
            curY += b.h + 10.0f;
        }
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        // Render Frame
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 10.0f, 1.0f, 0.18f, 0.19f, 0.2f, 1.0f);
        
        // Header Bar
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y, m_bounds.w, 30, 10.0f, 0.5f, 0.25f, 0.26f, 0.28f, 1.0f);
        batcher.drawText(m_name, m_bounds.x + 10, m_bounds.y + 8, 12, 0.9f, 0.9f, 0.9f, 1.0f);
        
        // Close Button (Hidden for Master)
        if (m_name != "Master") {
            batcher.drawRoundedRect(m_deleteBtnBounds.x, m_deleteBtnBounds.y, m_deleteBtnBounds.w, m_deleteBtnBounds.h, 2.0f, 0.5f, 0.6f, 0.2f, 0.2f, 1.0f);
            batcher.drawText("x", m_deleteBtnBounds.x + 4, m_deleteBtnBounds.y + 2, 10, 1.0f, 1.0f, 1.0f, 1.0f);
        }

        // Inner Content Area
        batcher.drawRoundedRect(m_bounds.x + 6, m_bounds.y + 35, m_bounds.w - 12, m_bounds.h - 41, 5.0f, 1.0f, 0.12f, 0.12f, 0.13f, 1.0f);

        // Update Dynamic Components (like Meters)
        if (m_name == "Audio Input") {
            auto inputNode = std::dynamic_pointer_cast<InputNode>(m_node);
            for(auto& child : m_children) {
                if (child->getName() == "LevelMeter") {
                    auto meter = std::dynamic_pointer_cast<LuminousMeter>(child);
                    if (meter && inputNode) meter->setLevel(inputNode->getPeakLevel());
                }
            }
        }

        // Render Ports and Children
        if (m_inputPort) m_inputPort->render(batcher, dt, screenW, screenH);
        if (m_outputPort) m_outputPort->render(batcher, dt, screenW, screenH);
        for (auto& child : m_children) child->render(batcher, dt, screenW, screenH);
    }

    bool onMouseDown(float x, float y, int button) override {
        if (m_name != "Master" && m_deleteBtnBounds.contains(x, y)) {
            if (onDeleteRequested) onDeleteRequested(this);
            return true;
        }
        
        if (m_inputPort && m_inputPort->onMouseDown(x, y, button)) return true;
        if (m_outputPort && m_outputPort->onMouseDown(x, y, button)) return true;
        
        for (auto& child : m_children) {
            if (child->getBounds().contains(x, y)) {
                if (child->onMouseDown(x, y, button)) return true;
            }
        }
        
        if (y < m_bounds.y + 25) { 
            startDragging(x, y);
            return true;
        }
        
        return false;
    }

    bool onMouseUp(float x, float y, int button) override {
        for (auto& child : m_children) child->onMouseUp(x, y, button);
        Component::onMouseUp(x, y, button);
        return true;
    }

    bool onMouseMove(float x, float y) override {
        for (auto& child : m_children) {
            if (child->onMouseMove(x, y)) return true;
        }
        return Component::onMouseMove(x, y);
    }

    void addChild(std::shared_ptr<Component> child) { m_children.push_back(child); }
    std::shared_ptr<Port> getInputPort() { return m_inputPort; }
    std::shared_ptr<Port> getOutputPort() { return m_outputPort; }

    std::function<void(AudioModule*)> onDeleteRequested;

protected:
    std::vector<std::shared_ptr<Component>> m_children;
    std::shared_ptr<FluxNode> m_node;
    std::shared_ptr<Port> m_inputPort;
    std::shared_ptr<Port> m_outputPort;

private:
    size_t m_nodeId;
    std::string m_name;
    Rect m_deleteBtnBounds;
};

} // namespace Beam

#endif