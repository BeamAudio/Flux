#ifndef TIMELINE_HPP
#define WORKSPACE_HPP // Wait, guard should be unique
#define TIMELINE_HPP

#include "component.hpp"
#include <vector>

namespace Beam {

class Timeline : public Component {
public:
    Timeline() {
        m_isVisible = false; // Hidden by default (start in Flux mode)
    }

    void render(QuadBatcher& batcher) override {
        if (!m_isVisible) return;

        // Draw Timeline Background (Darker)
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 0.05f, 0.05f, 0.06f, 1.0f);

        // Draw horizontal lanes for tracks
        float trackHeight = 100.0f;
        for (int i = 0; i < 5; ++i) {
            float y = m_bounds.y + (i * trackHeight);
            // Lane separator
            batcher.drawQuad(m_bounds.x, y + trackHeight - 1, m_bounds.w, 1, 0.2f, 0.2f, 0.2f, 1.0f);
            
            // Placeholder Waveform Clip
            batcher.drawQuad(m_bounds.x + 50, y + 10, 400, trackHeight - 20, 0.3f, 0.4f, 0.6f, 1.0f);
        }

        // Playhead
        batcher.drawQuad(m_bounds.x + 200, m_bounds.y, 2, m_bounds.h, 1.0f, 0.0f, 0.0f, 1.0f);
    }

    void setVisible(bool visible) { m_isVisible = visible; }

private:
    bool m_isVisible = false;
};

} // namespace Beam

#endif // TIMELINE_HPP
