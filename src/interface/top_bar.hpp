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

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        // Dark background for top bar
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 0.1f, 0.1f, 0.11f, 1.0f);
        // Soft accent line at bottom
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y + m_bounds.h - 3, m_bounds.w, 3, 1.5f, 0.5f, 0.2f, 0.4f, 0.8f, 1.0f);
        
        // Mode Buttons
        // Flux Button
        batcher.drawRoundedRect(10, 8, 70, 24, 4.0f, 0.5f, 0.18f, 0.18f, 0.22f, 1.0f);
        batcher.drawText("FLUX", 25, 12, 12, 1.0f, 1.0f, 1.0f, 1.0f);
        // Slice Button
        batcher.drawRoundedRect(85, 8, 70, 24, 4.0f, 0.5f, 0.18f, 0.18f, 0.22f, 1.0f);
        batcher.drawText("SLICE", 95, 12, 12, 1.0f, 1.0f, 1.0f, 1.0f);

        // Config Button
        batcher.drawRoundedRect(160, 8, 70, 24, 4.0f, 0.5f, 0.18f, 0.18f, 0.22f, 1.0f);
        batcher.drawText("CONFIG", 175, 12, 12, 1.0f, 1.0f, 1.0f, 1.0f);

        // Transport Controls (Center)
        float cx = m_bounds.w * 0.5f - 100;
        // Rewind
        batcher.drawRoundedRect(cx, 8, 40, 24, 4.0f, 0.5f, 0.25f, 0.25f, 0.25f, 1.0f);
        batcher.drawText("<<", cx + 10, 12, 12, 1.0f, 1.0f, 1.0f, 1.0f);
        // Play
        batcher.drawRoundedRect(cx + 45, 8, 40, 24, 4.0f, 0.5f, m_isPlaying ? 0.25f : 0.18f, m_isPlaying ? 0.7f : 0.35f, 0.25f, 1.0f);
        batcher.drawText(">", cx + 60, 12, 12, 1.0f, 1.0f, 1.0f, 1.0f);
        // Pause
        batcher.drawRoundedRect(cx + 90, 8, 40, 24, 4.0f, 0.5f, !m_isPlaying ? 0.25f : 0.18f, !m_isPlaying ? 0.7f : 0.35f, 0.25f, 1.0f);
        batcher.drawText("||", cx + 102, 12, 12, 1.0f, 1.0f, 1.0f, 1.0f);

        // Record
        batcher.drawRoundedRect(cx + 135, 8, 40, 24, 4.0f, 0.5f, m_isRecording ? 0.9f : 0.3f, 0.1f, 0.1f, 1.0f);
        batcher.drawText("O", cx + 150, 12, 12, 1.0f, 1.0f, 1.0f, 1.0f);

        // Save/Load Buttons (Right side)
        float rx = m_bounds.w - 240;
        batcher.drawRoundedRect(rx, 8, 70, 24, 4.0f, 0.5f, 0.18f, 0.25f, 0.18f, 1.0f); // Save
        batcher.drawText("SAVE", rx + 18, 12, 12, 1.0f, 1.0f, 1.0f, 1.0f);
        batcher.drawRoundedRect(rx + 75, 8, 70, 24, 4.0f, 0.5f, 0.18f, 0.18f, 0.25f, 1.0f); // Load
        batcher.drawText("LOAD", rx + 93, 12, 12, 1.0f, 1.0f, 1.0f, 1.0f);

        // Render Button
        float rBtnX = m_bounds.w - 80;
        batcher.drawRoundedRect(rBtnX, 8, 70, 24, 4.0f, 0.5f, 0.6f, 0.2f, 0.2f, 1.0f);
        batcher.drawText("RENDER", rBtnX + 10, 12, 12, 1.0f, 1.0f, 1.0f, 1.0f);

        // Timeline Tools (Only if in Slice mode)
        if (m_mode == 1) { // Slice
            float tx = 240;
            auto drawTool = [&](const std::string& lbl, int toolIdx) {
                bool active = (m_activeTool == toolIdx);
                batcher.drawRoundedRect(tx, 8, 30, 24, 4.0f, 0.5f, active ? 0.3f : 0.15f, active ? 0.6f : 0.16f, active ? 1.0f : 0.17f, 1.0f);
                batcher.drawText(lbl, tx + 8, 12, 12, 1.0f, 1.0f, 1.0f, 1.0f);
                tx += 35;
            };
            drawTool("P", 0); // Pointer
            drawTool("S", 1); // Scissors
            drawTool("G", 2); // Glue
        }
    }

    bool onMouseDown(float x, float y, int button) override {
        if (y > 8 && y < 32) {
            if (x > 10 && x < 80) { m_mode = 0; if (onModeChanged) onModeChanged(0); return true; }
            if (x > 85 && x < 155) { m_mode = 1; if (onModeChanged) onModeChanged(1); return true; }
            if (x > 160 && x < 230) { if (onConfigRequested) onConfigRequested(); return true; }
            
            if (m_mode == 1) {
                if (x > 240 && x < 270) { m_activeTool = 0; if (onToolSelected) onToolSelected(0); return true; }
                if (x > 275 && x < 305) { m_activeTool = 1; if (onToolSelected) onToolSelected(1); return true; }
                if (x > 310 && x < 340) { m_activeTool = 2; if (onToolSelected) onToolSelected(2); return true; }
            }
            
            float cx = m_bounds.w * 0.5f - 100;
            if (x > cx && x < cx + 40) { if (onRewindRequested) onRewindRequested(); return true; }
            if (x > cx + 45 && x < cx + 85) { setPlaying(true); if (onPlayRequested) onPlayRequested(); return true; }
            if (x > cx + 90 && x < cx + 130) { setPlaying(false); if (onPauseRequested) onPauseRequested(); return true; }
            if (x > cx + 135 && x < cx + 175) { 
                setRecording(!m_isRecording); 
                if (onRecordRequested) onRecordRequested(m_isRecording); 
                return true; 
            }

            float rx = m_bounds.w - 240;
            if (x > rx && x < rx + 70) { if (onSaveRequested) onSaveRequested(); return true; }
            if (x > rx + 75 && x < rx + 145) { if (onLoadRequested) onLoadRequested(); return true; }
            
            float rBtnX = m_bounds.w - 80;
            if (x > rBtnX && x < rBtnX + 70) { if (onRenderRequested) onRenderRequested(); return true; }
        }
        return false;
    }

    void setPlaying(bool playing) { m_isPlaying = playing; }
    void setRecording(bool recording) { m_isRecording = recording; }

    std::function<void(int)> onModeChanged;
    std::function<void()> onConfigRequested;
    std::function<void()> onSaveRequested;
    std::function<void()> onLoadRequested;
    std::function<void()> onPlayRequested;
    std::function<void()> onPauseRequested;
    std::function<void()> onRewindRequested;
    std::function<void(bool)> onRecordRequested;
    std::function<void()> onRenderRequested;
    std::function<void(int)> onToolSelected;

private:
    bool m_isPlaying = false;
    bool m_isRecording = false;
    int m_mode = 0; // 0: Flux, 1: Slice
    int m_activeTool = 0; // 0: Pointer, 1: Scissors, 2: Glue
};

} // namespace Beam

#endif // TOP_BAR_HPP






