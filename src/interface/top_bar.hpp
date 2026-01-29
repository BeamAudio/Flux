#ifndef TOP_BAR_HPP
#define TOP_BAR_HPP

#include "component.hpp"
#include "../utilities/flux_audio_utils.hpp"
#include <string>
#include <functional>
#include <vector>

namespace Beam {

class TopBar : public Component {
public:
    TopBar(int width) {
        setBounds(0, 0, (float)width, 40);
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 0.1f, 0.1f, 0.11f, 1.0f);
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y + m_bounds.h - 3, m_bounds.w, 3, 1.5f, 0.5f, 0.2f, 0.4f, 0.8f, 1.0f);
        
        float curX = 10.0f;
        float btnY = 8.0f;

        auto drawBtn = [&](const std::string& text, bool active) {
            float tw = AudioUtils::calculateTextWidth(text, 12.0f);
            float bw = tw + 20.0f;
            batcher.drawRoundedRect(curX, btnY, bw, 24, 4.0f, 0.5f, active ? 0.3f : 0.18f, active ? 0.5f : 0.18f, active ? 0.8f : 0.22f, 1.0f);
            batcher.drawText(text, curX + 10, btnY + 4, 12, 1.0f, 1.0f, 1.0f, 1.0f);
            float startX = curX;
            curX += bw + 10.0f;
            return Rect{startX, btnY, bw, 24};
        };

        m_btnFlux = drawBtn("FLUX", m_mode == 0);
        m_btnSlice = drawBtn("SLICE", m_mode == 1);
        m_btnConfig = drawBtn("CONFIG", false);

        // Tools (if in slice)
        if (m_mode == 1) {
            curX += 20.0f; // Gap
            m_btnP = drawBtn("P", m_activeTool == 0);
            m_btnS = drawBtn("S", m_activeTool == 1);
            m_btnG = drawBtn("G", m_activeTool == 2);
        }

        // Transport (Center)
        float transportW = 200.0f;
        float tx = m_bounds.w * 0.5f - transportW * 0.5f;
        auto drawTransport = [&](const std::string& text, float x, float w, bool active) {
            batcher.drawRoundedRect(x, btnY, w, 24, 4.0f, 0.5f, active ? 0.4f : 0.25f, active ? 0.4f : 0.25f, active ? 0.4f : 0.25f, 1.0f);
            batcher.drawText(text, x + (w - text.length()*12)/2, btnY + 4, 12, 1.0f, 1.0f, 1.0f, 1.0f);
            return Rect{x, btnY, w, 24};
        };

        m_btnRewind = drawTransport("<<", tx, 40, false);
        m_btnPlay = drawTransport(">", tx + 45, 40, m_isPlaying);
        m_btnPause = drawTransport("||", tx + 90, 40, !m_isPlaying);
        
        // Record (Red)
        float rCol = m_isRecording ? 0.9f : 0.3f;
        batcher.drawRoundedRect(tx + 135, btnY, 40, 24, 4.0f, 0.5f, rCol, 0.1f, 0.1f, 1.0f);
        batcher.drawText("O", tx + 135 + (40 - 12)/2, btnY + 4, 12, 1.0f, 1.0f, 1.0f, 1.0f);
        m_btnRecord = {tx + 135, btnY, 40, 24};

        // Right Side (Save/Load/Render) - Reverse order
        float rx = m_bounds.w - 10.0f;
        auto drawBtnRight = [&](const std::string& text, float& xRef) {
            float tw = AudioUtils::calculateTextWidth(text, 12.0f);
            float bw = tw + 20.0f;
            xRef -= bw;
            batcher.drawRoundedRect(xRef, btnY, bw, 24, 4.0f, 0.5f, 0.18f, 0.22f, 0.18f, 1.0f);
            batcher.drawText(text, xRef + 10, btnY + 4, 12, 1.0f, 1.0f, 1.0f, 1.0f);
            Rect r = {xRef, btnY, bw, 24};
            xRef -= 10.0f;
            return r;
        };

        m_btnRender = drawBtnRight("RENDER", rx);
        m_btnLoad = drawBtnRight("LOAD", rx);
        m_btnSave = drawBtnRight("SAVE", rx);
    }

    bool onMouseDown(float x, float y, int button) override {
        if (m_btnFlux.contains(x, y)) { m_mode = 0; if (onModeChanged) onModeChanged(0); return true; }
        if (m_btnSlice.contains(x, y)) { m_mode = 1; if (onModeChanged) onModeChanged(1); return true; }
        if (m_btnConfig.contains(x, y)) { if (onConfigRequested) onConfigRequested(); return true; }
        
        if (m_mode == 1) {
            if (m_btnP.contains(x, y)) { m_activeTool = 0; if (onToolSelected) onToolSelected(0); return true; }
            if (m_btnS.contains(x, y)) { m_activeTool = 1; if (onToolSelected) onToolSelected(1); return true; }
            if (m_btnG.contains(x, y)) { m_activeTool = 2; if (onToolSelected) onToolSelected(2); return true; }
        }

        if (m_btnRewind.contains(x, y)) { if (onRewindRequested) onRewindRequested(); return true; }
        if (m_btnPlay.contains(x, y)) { setPlaying(true); if (onPlayRequested) onPlayRequested(); return true; }
        if (m_btnPause.contains(x, y)) { setPlaying(false); if (onPauseRequested) onPauseRequested(); return true; }
        if (m_btnRecord.contains(x, y)) { setRecording(!m_isRecording); if (onRecordRequested) onRecordRequested(m_isRecording); return true; }

        if (m_btnSave.contains(x, y)) { if (onSaveRequested) onSaveRequested(); return true; }
        if (m_btnLoad.contains(x, y)) { if (onLoadRequested) onLoadRequested(); return true; }
        if (m_btnRender.contains(x, y)) { if (onRenderRequested) onRenderRequested(); return true; }

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
    int m_mode = 0;
    int m_activeTool = 0;
    
    Rect m_btnFlux, m_btnSlice, m_btnConfig;
    Rect m_btnP, m_btnS, m_btnG;
    Rect m_btnRewind, m_btnPlay, m_btnPause, m_btnRecord;
    Rect m_btnSave, m_btnLoad, m_btnRender;
};

} // namespace Beam

#endif