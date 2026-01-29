#ifndef PORT_HPP
#define PORT_HPP

#include "component.hpp"
#include <functional>

namespace Beam {

enum class PortType { Input, Output };

class Port : public Component {
public:
    Port(PortType type, class AudioModule* parent) : m_type(type), m_parent(parent) {
        setBounds(0, 0, 12, 12);
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        float r = (m_type == PortType::Input) ? 0.4f : 0.8f;
        float g = (m_type == PortType::Input) ? 0.8f : 0.4f;
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, r, g, 1.0f, 1.0f);
    }

    bool onMouseDown(float x, float y, int button) override {
        Rect hitArea = { m_bounds.x - 10, m_bounds.y - 10, m_bounds.w + 20, m_bounds.h + 20 };
        if (hitArea.contains(x, y)) {
            if (onConnectStarted) onConnectStarted(this);
            return true;
        }
        return false;
    }

    PortType getType() const { return m_type; }
    class AudioModule* getModule() const { return m_parent; }

    std::function<void(Port*)> onConnectStarted;

private:
    PortType m_type;
    class AudioModule* m_parent;
};

} // namespace Beam

#endif // PORT_HPP





