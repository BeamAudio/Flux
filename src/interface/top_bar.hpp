#ifndef TOP_BAR_HPP
#define TOP_BAR_HPP

#include "component.hpp"
#include "button.hpp"
#include "../utilities/flux_audio_utils.hpp"
#include <string>
#include <functional>
#include <vector>

namespace Beam {

class TopBar : public Component {
public:
    TopBar(int width) {
        setName("TopBar");
        
        setupButtons();
        setBounds(0, 0, (float)width, 40);
    }

    void setupButtons() {
        m_fluxBtn = std::make_shared<TextButton>("FLUX");
        m_sliceBtn = std::make_shared<TextButton>("SLICE");
        m_configBtn = std::make_shared<TextButton>("CONFIG");
        
        m_pointerBtn = std::make_shared<ToggleButton>("PTR");
        m_scissorsBtn = std::make_shared<ToggleButton>("CUT");
        m_glueBtn = std::make_shared<ToggleButton>("GLUE");

        m_rewindBtn = std::make_shared<TextButton>("<<");
        m_playBtn = std::make_shared<TextButton>(">");
        m_pauseBtn = std::make_shared<TextButton>("||");
        m_recordBtn = std::make_shared<ToggleButton>("O");

        m_saveBtn = std::make_shared<TextButton>("SAVE");
        m_loadBtn = std::make_shared<TextButton>("LOAD");
        m_renderBtn = std::make_shared<TextButton>("RENDER");

        addChildComponent(m_fluxBtn);
        addChildComponent(m_sliceBtn);
        addChildComponent(m_configBtn);
        
        addChildComponent(m_pointerBtn);
        addChildComponent(m_scissorsBtn);
        addChildComponent(m_glueBtn);

        addChildComponent(m_rewindBtn);
        addChildComponent(m_playBtn);
        addChildComponent(m_pauseBtn);
        addChildComponent(m_recordBtn);
        addChildComponent(m_saveBtn);
        addChildComponent(m_loadBtn);
        addChildComponent(m_renderBtn);

        m_fluxBtn->onClick([this]() { m_mode = 0; if (onModeChanged) onModeChanged(0); updateButtonStates(); });
        m_sliceBtn->onClick([this]() { m_mode = 1; if (onModeChanged) onModeChanged(1); updateButtonStates(); });
        m_configBtn->onClick([this]() { if (onConfigRequested) onConfigRequested(); });
        
        m_pointerBtn->onClick([this]() { setTool(0); });
        m_scissorsBtn->onClick([this]() { setTool(1); });
        m_glueBtn->onClick([this]() { setTool(2); });

        m_rewindBtn->onClick([this]() { if (onRewindRequested) onRewindRequested(); });
        m_playBtn->onClick([this]() { setPlaying(true); if (onPlayRequested) onPlayRequested(); });
        m_pauseBtn->onClick([this]() { setPlaying(false); if (onPauseRequested) onPauseRequested(); });
        m_recordBtn->onClick([this]() { bool rec = m_recordBtn->getToggleState(); if (onRecordRequested) onRecordRequested(rec); });

        m_saveBtn->onClick([this]() { if (onSaveRequested) onSaveRequested(); });
        m_loadBtn->onClick([this]() { if (onLoadRequested) onLoadRequested(); });
        m_renderBtn->onClick([this]() { if (onRenderRequested) onRenderRequested(); });

        setTool(0); // Default to Pointer
        updateButtonStates();
    }

    void setTool(int tool) {
        m_tool = tool;
        m_pointerBtn->setToggleState(tool == 0, false);
        m_scissorsBtn->setToggleState(tool == 1, false);
        m_glueBtn->setToggleState(tool == 2, false);
        if (onToolSelected) onToolSelected(tool);
    }

    void updateButtonStates() {
        m_fluxBtn->setToggleState(m_mode == 0, false);
        m_sliceBtn->setToggleState(m_mode == 1, false);
        m_playBtn->setToggleState(m_isPlaying, false);
        m_pauseBtn->setToggleState(!m_isPlaying, false);
        
        // Hide/Show tools based on mode? Or just enable/disable?
        // For simplicity, let's keep them visible but layout them only if in Slice mode?
        // Layout handles visibility indirectly if we move them offscreen, but setVisible is better.
        bool sliceMode = (m_mode == 1);
        m_pointerBtn->setVisible(sliceMode);
        m_scissorsBtn->setVisible(sliceMode);
        m_glueBtn->setVisible(sliceMode);
    }

    void resized() override {
        float x = m_bounds.x;
        float y = m_bounds.y;
        float curX = x + 10.0f;
        float btnY = y + 8.0f;
        float btnH = 24.0f;

        auto layoutBtn = [&](std::shared_ptr<Button> btn) {
            float tw = AudioUtils::calculateTextWidth(btn->getButtonText(), 12.0f);
            float bw = tw + 20.0f;
            btn->setBounds(curX, btnY, bw, btnH);
            curX += bw + 10.0f;
        };

        layoutBtn(m_fluxBtn);
        layoutBtn(m_sliceBtn);
        layoutBtn(m_configBtn);
        
        curX += 20.0f; // Spacer
        
        if (m_mode == 1) { // Only layout tools in Slice Mode
            layoutBtn(m_pointerBtn);
            layoutBtn(m_scissorsBtn);
            layoutBtn(m_glueBtn);
        }

        float tx = x + m_bounds.w * 0.5f - 100.0f;
        m_rewindBtn->setBounds(tx, btnY, 40, btnH);
        m_playBtn->setBounds(tx + 45, btnY, 40, btnH);
        m_pauseBtn->setBounds(tx + 90, btnY, 40, btnH);
        m_recordBtn->setBounds(tx + 135, btnY, 40, btnH);

        float rx = x + m_bounds.w - 10.0f;
        auto layoutBtnRight = [&](std::shared_ptr<Button> btn) {
            float tw = AudioUtils::calculateTextWidth(btn->getButtonText(), 12.0f);
            float bw = tw + 20.0f;
            rx -= bw;
            btn->setBounds(rx, btnY, bw, btnH);
            rx -= 10.0f;
        };

        layoutBtnRight(m_renderBtn);
        layoutBtnRight(m_loadBtn);
        layoutBtnRight(m_saveBtn);
    }

    void paint(QuadBatcher& batcher) override {
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 0.05f, 0.05f, 0.06f, 1.0f); // BRAND_BLACK
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y + m_bounds.h - 3, m_bounds.w, 3, 1.5f, 0.5f, 0.13f, 0.62f, 0.42f, 1.0f); // BRAND_EMERALD
    }

    void setPlaying(bool playing) { m_isPlaying = playing; updateButtonStates(); }
    void setRecording(bool recording) { m_recordBtn->setToggleState(recording, false); }

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
    int m_mode = 0;
    int m_tool = 0;
    
    std::shared_ptr<TextButton> m_fluxBtn, m_sliceBtn, m_configBtn;
    std::shared_ptr<ToggleButton> m_pointerBtn, m_scissorsBtn, m_glueBtn;
    std::shared_ptr<TextButton> m_rewindBtn, m_playBtn, m_pauseBtn;
    std::shared_ptr<ToggleButton> m_recordBtn;
    std::shared_ptr<TextButton> m_saveBtn, m_loadBtn, m_renderBtn;
};

} // namespace Beam

#endif