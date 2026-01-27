#ifndef AUDIO_MODULE_HPP
#define AUDIO_MODULE_HPP

#include "component.hpp"
#include "knob.hpp"
#include "port.hpp"
#include <string>
#include <vector>

namespace Beam {

class AudioModule : public Component {
public:
    AudioModule(const std::string& name, float x, float y) : m_name(name) {
        setBounds(x, y, 150, 200);
        setDraggable(true);

        m_inputPort = std::make_shared<Port>(PortType::Input, this);
        m_outputPort = std::make_shared<Port>(PortType::Output, this);
    }

    void setBounds(float x, float y, float w, float h) {
        Component::setBounds(x, y, w, h);
        // Update port positions relative to module
        m_inputPort->setBounds(x - 6, y + 50, 12, 12);
        m_outputPort->setBounds(x + w - 6, y + 50, 12, 12);
    }

    void render(QuadBatcher& batcher) override {
        // Module Body
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 0.22f, 0.23f, 0.24f, 1.0f);
        // Header
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, 25, 0.3f, 0.35f, 0.4f, 1.0f);
        
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
    std::string m_name;
    std::vector<std::shared_ptr<Component>> m_children;
    std::shared_ptr<Port> m_inputPort;
    std::shared_ptr<Port> m_outputPort;
};

} // namespace Beam

#endif // AUDIO_MODULE_HPP
