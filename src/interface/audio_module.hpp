#ifndef AUDIO_MODULE_HPP
#define AUDIO_MODULE_HPP

#include "component.hpp"
#include "knob.hpp"
#include "port.hpp"
#include "../engine/flux_node.hpp"
#include <string>
#include <vector>
#include <functional>

namespace Beam {

class AudioModule : public Component {
public:
    AudioModule(std::shared_ptr<FluxNode> node, size_t nodeId, float x, float y) 
        : m_node(node), m_nodeId(nodeId), m_name(node->getName()) {
        
        if (node->getInputPorts().size() > 0)
            m_inputPort = std::make_shared<Port>(PortType::Input, this);
            
        if (node->getOutputPorts().size() > 0)
            m_outputPort = std::make_shared<Port>(PortType::Output, this);
        
        setBounds(x, y, 150, 200);
        setDraggable(true);
        
        autoGenerateUI();
    }

    size_t getNodeId() const { return m_nodeId; }

    void autoGenerateUI() {
        if (!m_node) return;
        float startY = 40;
        float spacing = 60;
        int count = 0;
        for (auto const& [name, param] : m_node->getParameters()) {
            if (name == "Master Gain") continue; // Skip master gain in module auto-ui
            auto knob = std::make_shared<Knob>(name, param->getMin(), param->getMax(), param->getValue());
            knob->bindParameter(param);
            knob->setBounds(m_bounds.x + 25, m_bounds.y + startY + (count * spacing), 100, 40);
            addChild(knob);
            count++;
        }
        if (count > 0) {
            float newHeight = startY + (count * spacing) + 20;
            if (newHeight > m_bounds.h) setBounds(m_bounds.x, m_bounds.y, m_bounds.w, newHeight);
        }
    }

    void setBounds(float x, float y, float w, float h) override {
        float dx = x - m_bounds.x;
        float dy = y - m_bounds.y;
        Component::setBounds(x, y, w, h);
        if (m_inputPort) m_inputPort->setBounds(x - 6, y + 50, 12, 12);
        if (m_outputPort) m_outputPort->setBounds(x + w - 6, y + 50, 12, 12);
        m_deleteBtnBounds = {x + w - 20, y + 5, 15, 15};
        for (auto& child : m_children) {
            Rect b = child->getBounds();
            child->setBounds(b.x + dx, b.y + dy, b.w, b.h);
        }
    }

    void render(QuadBatcher& batcher, float dt) override {
        // Module Body
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 10.0f, 1.0f, 0.18f, 0.19f, 0.2f, 1.0f);
        // Header
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y, m_bounds.w, 30, 10.0f, 0.5f, 0.25f, 0.26f, 0.28f, 1.0f);
        batcher.drawText(m_name, m_bounds.x + 10, m_bounds.y + 8, 14, 0.9f, 0.9f, 0.9f, 1.0f);
        
        // Delete Button (X)
        batcher.drawRoundedRect(m_deleteBtnBounds.x, m_deleteBtnBounds.y, m_deleteBtnBounds.w, m_deleteBtnBounds.h, 2.0f, 0.5f, 0.6f, 0.2f, 0.2f, 1.0f);
        batcher.drawText("x", m_deleteBtnBounds.x + 4, m_deleteBtnBounds.y + 2, 10, 1.0f, 1.0f, 1.0f, 1.0f);

        // Internal panel
        batcher.drawRoundedRect(m_bounds.x + 10, m_bounds.y + 40, m_bounds.w - 20, m_bounds.h - 50, 5.0f, 2.0f, 0.12f, 0.13f, 0.14f, 1.0f);
        
        if (m_inputPort) m_inputPort->render(batcher, dt);
        if (m_outputPort) m_outputPort->render(batcher, dt);
        for (auto& child : m_children) child->render(batcher, dt);
    }

    bool onMouseDown(float x, float y, int button) override {
        if (m_deleteBtnBounds.contains(x, y)) {
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
        bool handled = false;
        for (auto& child : m_children) {
            if (child->onMouseUp(x, y, button)) handled = true;
        }
        Component::onMouseUp(x, y, button);
        return handled;
    }

    bool onMouseMove(float x, float y) override {
        for (auto& child : m_children) {
            if (child->onMouseMove(x, y)) return true;
        }

        bool changed = Component::onMouseMove(x, y);
        if (changed) {
            if (m_inputPort) m_inputPort->setBounds(m_bounds.x - 6, m_bounds.y + 50, 12, 12);
            if (m_outputPort) m_outputPort->setBounds(m_bounds.x + m_bounds.w - 6, m_bounds.y + 50, 12, 12);
            m_deleteBtnBounds = {m_bounds.x + m_bounds.w - 20, m_bounds.y + 5, 15, 15};
        }
        return changed;
    }

    void addChild(std::shared_ptr<Component> child) { m_children.push_back(child); }
    std::shared_ptr<Port> getInputPort() { return m_inputPort; }
    std::shared_ptr<Port> getOutputPort() { return m_outputPort; }

    std::function<void(AudioModule*)> onDeleteRequested;

private:
    std::shared_ptr<FluxNode> m_node;
    size_t m_nodeId;
    std::string m_name;
    std::vector<std::shared_ptr<Component>> m_children;
    std::shared_ptr<Port> m_inputPort;
    std::shared_ptr<Port> m_outputPort;
    Rect m_deleteBtnBounds;
};

} // namespace Beam

#endif // AUDIO_MODULE_HPP



