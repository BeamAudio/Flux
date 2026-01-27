#ifndef AUDIO_MODULE_HPP
#define AUDIO_MODULE_HPP

#include "component.hpp"
#include "knob.hpp"
#include "port.hpp"
#include "../dsp/flux_node.hpp"
#include <string>
#include <vector>

namespace Beam {

class AudioModule : public Component {
public:
    AudioModule(std::shared_ptr<FluxNode> node, float x, float y) 
        : m_node(node), m_name(node->getName()) {
        m_inputPort = std::make_shared<Port>(PortType::Input, this);
        m_outputPort = std::make_shared<Port>(PortType::Output, this);
        
        setBounds(x, y, 150, 200);
        setDraggable(true);
        
        autoGenerateUI();
    }

    void autoGenerateUI() {
        if (!m_node) return;

        float startY = 40;
        float spacing = 60;
        int count = 0;

        for (auto const& [name, param] : m_node->getParameters()) {
            auto knob = std::make_shared<Knob>(name, param->getMin(), param->getMax(), param->getValue());
            knob->bindParameter(param);
            knob->setBounds(m_bounds.x + 25, m_bounds.y + startY + (count * spacing), 100, 40);
            addChild(knob);
            count++;
        }

        // Adjust module height based on number of knobs
        if (count > 0) {
            float newHeight = startY + (count * spacing) + 20;
            if (newHeight > m_bounds.h) {
                setBounds(m_bounds.x, m_bounds.y, m_bounds.w, newHeight);
            }
        }
    }

    void setBounds(float x, float y, float w, float h) override {
        float dx = x - m_bounds.x;
        float dy = y - m_bounds.y;
        
        Component::setBounds(x, y, w, h);
        
        // Update port positions relative to module
        m_inputPort->setBounds(x - 6, y + 50, 12, 12);
        m_outputPort->setBounds(x + w - 6, y + 50, 12, 12);

        // Update children positions
        for (auto& child : m_children) {
            Rect b = child->getBounds();
            child->setBounds(b.x + dx, b.y + dy, b.w, b.h);
        }
    }

    void render(QuadBatcher& batcher) override {
        // Module Body
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 0.22f, 0.23f, 0.24f, 1.0f);
        // Header
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, 25, 0.3f, 0.35f, 0.4f, 1.0f);
        batcher.drawText(m_name, m_bounds.x + 5, m_bounds.y + 5, 14, 0.9f, 0.9f, 0.9f, 1.0f);
        
        m_inputPort->render(batcher);
        m_outputPort->render(batcher);

        for (auto& child : m_children) {
            child->render(batcher);
        }
    }

    bool onMouseDown(float x, float y, int button) override {
        if (m_inputPort->onMouseDown(x, y, button)) return true;
        if (m_outputPort->onMouseDown(x, y, button)) return true;

        for (auto& child : m_children) {
            if (child->getBounds().contains(x, y)) {
                return child->onMouseDown(x, y, button);
            }
        }

        if (y < m_bounds.y + 25) { 
            startDragging(x, y);
            return true;
        }
        return false;
    }

    bool onMouseMove(float x, float y) override {
        bool changed = Component::onMouseMove(x, y);
        if (changed) {
            // Update port absolute positions when module moves
            m_inputPort->setBounds(m_bounds.x - 6, m_bounds.y + 50, 12, 12);
            m_outputPort->setBounds(m_bounds.x + m_bounds.w - 6, m_bounds.y + 50, 12, 12);
        }
        return changed;
    }

    void addChild(std::shared_ptr<Component> child) {
        m_children.push_back(child);
    }

    std::shared_ptr<Port> getInputPort() { return m_inputPort; }
    std::shared_ptr<Port> getOutputPort() { return m_outputPort; }

private:
    std::shared_ptr<FluxNode> m_node;
    std::string m_name;
    std::vector<std::shared_ptr<Component>> m_children;
    std::shared_ptr<Port> m_inputPort;
    std::shared_ptr<Port> m_outputPort;
};

} // namespace Beam

#endif // AUDIO_MODULE_HPP
