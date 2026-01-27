#ifndef TOP_BAR_HPP
#define TOP_BAR_HPP

#include "component.hpp"
#include <string>
#include <functional>

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
        
        // Mode Buttons
        // Flux Button
        batcher.drawQuad(10, 8, 70, 24, 0.2f, 0.2f, 0.25f, 1.0f);
        batcher.drawText("FLUX", 25, 12, 12, 1.0f, 1.0f, 1.0f, 1.0f);
        // Slice Button
        batcher.drawQuad(85, 8, 70, 24, 0.2f, 0.2f, 0.25f, 1.0f);
        batcher.drawText("SLICE", 95, 12, 12, 1.0f, 1.0f, 1.0f, 1.0f);

        // Transport Controls (Center)
        float cx = m_bounds.w * 0.5f - 100;
        // Rewind
        batcher.drawQuad(cx, 8, 40, 24, 0.3f, 0.3f, 0.3f, 1.0f);
        batcher.drawText("<<", cx + 10, 12, 12, 1.0f, 1.0f, 1.0f, 1.0f);
        // Play
        batcher.drawQuad(cx + 45, 8, 40, 24, m_isPlaying ? 0.3f : 0.2f, m_isPlaying ? 0.8f : 0.4f, 0.3f, 1.0f);
        batcher.drawText(">", cx + 60, 12, 12, 1.0f, 1.0f, 1.0f, 1.0f);
        // Pause
        batcher.drawQuad(cx + 90, 8, 40, 24, !m_isPlaying ? 0.3f : 0.2f, !m_isPlaying ? 0.8f : 0.4f, 0.3f, 1.0f);
        batcher.drawText("||", cx + 102, 12, 12, 1.0f, 1.0f, 1.0f, 1.0f);

        // Save/Load Buttons (Right side)
        float rx = m_bounds.w - 160;
        batcher.drawQuad(rx, 8, 70, 24, 0.2f, 0.3f, 0.2f, 1.0f); // Save
        batcher.drawText("SAVE", rx + 18, 12, 12, 1.0f, 1.0f, 1.0f, 1.0f);
        batcher.drawQuad(rx + 75, 8, 70, 24, 0.2f, 0.2f, 0.3f, 1.0f); // Load
        batcher.drawText("LOAD", rx + 93, 12, 12, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    bool onMouseDown(float x, float y, int button) override {
        if (y > 8 && y < 32) {
            if (x > 10 && x < 80) { if (onModeChanged) onModeChanged(0); return true; }
            if (x > 85 && x < 155) { if (onModeChanged) onModeChanged(1); return true; }
            
            float cx = m_bounds.w * 0.5f - 100;
            if (x > cx && x < cx + 40) { if (onRewindRequested) onRewindRequested(); return true; }
            if (x > cx + 45 && x < cx + 85) { setPlaying(true); if (onPlayRequested) onPlayRequested(); return true; }
            if (x > cx + 90 && x < cx + 130) { setPlaying(false); if (onPauseRequested) onPauseRequested(); return true; }

            float rx = m_bounds.w - 160;
            if (x > rx && x < rx + 70) { if (onSaveRequested) onSaveRequested(); return true; }
            if (x > rx + 75 && x < rx + 145) { if (onLoadRequested) onLoadRequested(); return true; }
        }
        return false;
    }

    void setPlaying(bool playing) { m_isPlaying = playing; }

    std::function<void(int)> onModeChanged;
    std::function<void()> onSaveRequested;
    std::function<void()> onLoadRequested;
    std::function<void()> onPlayRequested;
    std::function<void()> onPauseRequested;
    std::function<void()> onRewindRequested;

private:
    bool m_isPlaying = false;
};

} // namespace Beam

#endif // TOP_BAR_HPP
