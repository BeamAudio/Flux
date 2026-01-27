#ifndef SIDEBAR_HPP
#define SIDEBAR_HPP

#include "component.hpp"

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

        // Tab placeholders
        if (m_side == Side::Left) {
            batcher.drawQuad(m_bounds.x + 10, m_bounds.y + 50, m_bounds.w - 20, 30, 0.15f, 0.16f, 0.17f, 1.0f); // FX TAB
            batcher.drawQuad(m_bounds.x + 10, m_bounds.y + 90, m_bounds.w - 20, 30, 0.15f, 0.16f, 0.17f, 1.0f); // ASSETS TAB
        }
    }

private:
    Side m_side;
};

} // namespace Beam

#endif // SIDEBAR_HPP
