#ifndef TOP_BAR_HPP
#define TOP_BAR_HPP

#include "component.hpp"
#include <string>

namespace Beam {

class TopBar : public Component {
public:
    TopBar(int width) {
        setBounds(0, 0, (float)width, 40);
    }

    void render(QuadBatcher& batcher) override {
        // Dark background for top bar
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 0.12f, 0.13f, 0.14f, 1.0f);
        // Accent line at bottom
        batcher.drawQuad(m_bounds.x, m_bounds.y + m_bounds.h - 2, m_bounds.w, 2, 0.25f, 0.5f, 1.0f, 1.0f);
        
        // Logo Placeholder
        batcher.drawQuad(10, 10, 20, 20, 1.0f, 1.0f, 1.0f, 1.0f);
    }
};

} // namespace Beam

#endif // TOP_BAR_HPP
