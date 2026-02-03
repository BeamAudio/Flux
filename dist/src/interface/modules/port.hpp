#ifndef PORT_HPP
#define PORT_HPP

#include "interface/core/component.hpp"
#include <functional>

namespace Beam {

enum class PortType { Input, Output, Sidechain };

class Port : public Component {
public:
    Port(PortType type, Component* parent, int index = 0) : m_type(type), m_index(index) {
        setParent(parent);
        setName("Port");
        setBounds(0, 0, 12, 12);
    }

    void paint(QuadBatcher& batcher) override {
        float r = (m_type == PortType::Input) ? 0.4f : 0.8f;
        float g = (m_type == PortType::Input) ? 0.8f : 0.4f;
        float b = 1.0f;

        if (m_type == PortType::Sidechain) {
            r = 0.95f; g = 0.95f; b = 0.95f; // White for Sidechain
        }

        batcher.drawRoundedRect(0, 0, m_bounds.w, m_bounds.h, m_bounds.w*0.5f, 0.5f, r, g, b, 1.0f);
    }

    bool onMouseDown(float x, float y, int button, bool shift) override {
        // x,y are local. We add a hit margin.
        float margin = 5.0f;
        if (x >= -margin && x <= m_bounds.w + margin && y >= -margin && y <= m_bounds.h + margin) {
            if (onConnectStarted) onConnectStarted(this);
            return true;
        }
        return false;
    }

    PortType getType() const { return m_type; }
    int getIndex() const { return m_index; }

    std::function<void(Port*)> onConnectStarted;

private:
    PortType m_type;
    int m_index;
};

} // namespace Beam

#endif // PORT_HPP





