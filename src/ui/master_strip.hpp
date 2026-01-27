#ifndef MASTER_STRIP_HPP
#define MASTER_STRIP_HPP

#include "component.hpp"

namespace Beam {

class MasterStrip : public Component {
public:
    MasterStrip() {}

    void render(QuadBatcher& batcher) override {
        // Background
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 0.12f, 0.12f, 0.13f, 1.0f);
        
        // VU Meter area
        batcher.drawQuad(m_bounds.x + 10, m_bounds.y + 20, m_bounds.w - 20, 150, 0.05f, 0.05f, 0.05f, 1.0f);
        
        // Master Fader track
        batcher.drawQuad(m_bounds.x + m_bounds.w/2 - 2, m_bounds.y + 200, 4, m_bounds.h - 250, 0.0f, 0.0f, 0.0f, 1.0f);
        
        // Master Fader Cap (Red for Master)
        batcher.drawQuad(m_bounds.x + 15, m_bounds.y + 400, m_bounds.w - 30, 15, 0.8f, 0.2f, 0.2f, 1.0f);
    }
};

} // namespace Beam

#endif // MASTER_STRIP_HPP
