#ifndef PORT_HPP
#define PORT_HPP

#include "component.hpp"
#include <functional>

namespace Beam {

enum class PortType { Input, Output };

class Port : public Component {
public:
    Port(PortType type, Component* parent) : m_type(type), m_parent(parent) {
        setBounds(0, 0, 12, 12);
    }

    void render(QuadBatcher& batcher) override {
        // Ports are small circles (represented as quads for now)
        float r = (m_type == PortType::Input) ? 0.4f : 0.8f;
        float g = (m_type == PortType::Input) ? 0.8f : 0.4f;
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, r, g, 1.0f, 1.0f);
    }

    bool onMouseDown(float x, float y, int button) override {
        if (m_bounds.contains(x, y)) {
            if (onConnectStarted) onConnectStarted(this);
            return true;
        }
        return false;
    }

    PortType getType() const { return m_type; }
    Component* getParentModule() const { return m_parent; }

    std::function<void(Port*)> onConnectStarted;

private:
    PortType m_type;
    Component* m_parent;
};

} // namespace Beam

#endif // PORT_HPP
