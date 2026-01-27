#ifndef SIDEBAR_HPP
#define SIDEBAR_HPP

#include "component.hpp"
#include <functional>
#include <string>

namespace Beam {

class Sidebar : public Component {
public:
    enum class Side { Left, Right };

    Sidebar(Side side) : m_side(side) {
        // Initial width, height will be set by Layout Engine
    }

    void render(QuadBatcher& batcher) override {
        // Dark sidebar background
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 0.1f, 0.11f, 0.12f, 1.0f);
        
        // Edge border
        float borderX = (m_side == Side::Left) ? m_bounds.x + m_bounds.w - 1 : m_bounds.x;
        batcher.drawQuad(borderX, m_bounds.y, 1, m_bounds.h, 0.2f, 0.2f, 0.2f, 1.0f);

        // FX List
        if (m_side == Side::Left) {
            float yOff = m_bounds.y + 50;
            // Gain FX
            batcher.drawQuad(m_bounds.x + 10, yOff, m_bounds.w - 20, 30, 0.15f, 0.16f, 0.17f, 1.0f);
            batcher.drawText("GAIN", m_bounds.x + 20, yOff + 8, 14, 0.7f, 0.7f, 0.7f, 1.0f);
            yOff += 40;
            // Filter FX
            batcher.drawQuad(m_bounds.x + 10, yOff, m_bounds.w - 20, 30, 0.15f, 0.16f, 0.17f, 1.0f);
            batcher.drawText("FILTER", m_bounds.x + 20, yOff + 8, 14, 0.7f, 0.7f, 0.7f, 1.0f);
            yOff += 40;
            // Delay FX
            batcher.drawQuad(m_bounds.x + 10, yOff, m_bounds.w - 20, 30, 0.15f, 0.16f, 0.17f, 1.0f);
            batcher.drawText("DELAY", m_bounds.x + 20, yOff + 8, 14, 0.7f, 0.7f, 0.7f, 1.0f);
        }
    }

    bool onMouseDown(float x, float y, int button) override {
        if (m_side == Side::Left) {
            float yOff = m_bounds.y + 50;
            if (y > yOff && y < yOff + 30) { if (onAddFX) onAddFX("Gain"); return true; }
            yOff += 40;
            if (y > yOff && y < yOff + 30) { if (onAddFX) onAddFX("Filter"); return true; }
            yOff += 40;
            if (y > yOff && y < yOff + 30) { if (onAddFX) onAddFX("Delay"); return true; }
        }
        return false;
    }

    std::function<void(std::string)> onAddFX;

private:
    Side m_side;
};

} // namespace Beam

#endif // SIDEBAR_HPP
