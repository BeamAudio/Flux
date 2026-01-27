#ifndef WORKSPACE_HPP
#define WORKSPACE_HPP

#include "component.hpp"
#include <vector>

namespace Beam {

class Workspace : public Component {
public:
    Workspace() {
        setBounds(0, 0, 10000, 10000); // Massive canvas
    }

    void render(QuadBatcher& batcher) override {
        // Draw Grid (The "Infinite Floor")
        float spacing = 50.0f;
        for (float x = 0; x < 2000; x += spacing) {
            batcher.drawQuad(x + m_panX, 0, 1, 2000, 0.2f, 0.2f, 0.2f, 1.0f);
        }
        for (float y = 0; y < 2000; y += spacing) {
            batcher.drawQuad(0, y + m_panY, 2000, 1, 0.2f, 0.2f, 0.2f, 1.0f);
        }

        for (auto& module : m_modules) {
            module->render(batcher);
        }
    }

    void addModule(std::shared_ptr<Component> module) {
        m_modules.push_back(module);
    }

    bool onMouseDown(float x, float y, int button) override {
        for (auto it = m_modules.rbegin(); it != m_modules.rend(); ++it) {
            if ((*it)->getBounds().contains(x, y)) {
                return (*it)->onMouseDown(x, y, button);
            }
        }
        
        if (button == 2) { // Middle click pan
            m_isPanning = true;
            m_lastMouseX = x;
            m_lastMouseY = y;
            return true;
        }
        return false;
    }

    bool onMouseMove(float x, float y) override {
        if (m_isPanning) {
            m_panX += (x - m_lastMouseX);
            m_panY += (y - m_lastMouseY);
            m_lastMouseX = x;
            m_lastMouseY = y;
            return true;
        }

        for (auto& mod : m_modules) {
            if (mod->onMouseMove(x, y)) return true;
        }
        return false;
    }

    bool onMouseUp(float x, float y, int button) override {
        m_isPanning = false;
        for (auto& mod : m_modules) mod->onMouseUp(x, y, button);
        return true;
    }

private:
    std::vector<std::shared_ptr<Component>> m_modules;
    float m_panX = 0, m_panY = 0;
    bool m_isPanning = false;
    float m_lastMouseX = 0, m_lastMouseY = 0;
};

} // namespace Beam

#endif // WORKSPACE_HPP
