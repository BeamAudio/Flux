#ifndef AUDIO_MODULE_HPP
#define AUDIO_MODULE_HPP

#include "component.hpp"
#include "knob.hpp"
#include <string>
#include <vector>

namespace Beam {

class AudioModule : public Component {
public:
    AudioModule(const std::string& name, float x, float y) : m_name(name) {
        setBounds(x, y, 150, 200);
        setDraggable(true);
    }

    void render(QuadBatcher& batcher) override {
        // Module Body
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 0.22f, 0.23f, 0.24f, 1.0f);
        // Header
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, 25, 0.3f, 0.35f, 0.4f, 1.0f);
        
        // Input Port (Left)
        batcher.drawQuad(m_bounds.x - 5, m_bounds.y + 50, 10, 10, 0.8f, 0.8f, 0.8f, 1.0f);
        // Output Port (Right)
        batcher.drawQuad(m_bounds.x + m_bounds.w - 5, m_bounds.y + 50, 10, 10, 0.8f, 0.8f, 0.8f, 1.0f);

        for (auto& child : m_children) {
            child->render(batcher);
        }
    }

    bool onMouseDown(float x, float y, int button) override {
        // Check children first
        for (auto& child : m_children) {
            if (child->getBounds().contains(x, y)) {
                return child->onMouseDown(x, y, button);
            }
        }

        if (y < m_bounds.y + 25) { // Drag by header
            startDragging(x, y);
            return true;
        }
        return false;
    }

    void addChild(std::shared_ptr<Component> child) {
        m_children.push_back(child);
    }

private:
    std::string m_name;
    std::vector<std::shared_ptr<Component>> m_children;
};

} // namespace Beam

#endif // AUDIO_MODULE_HPP
