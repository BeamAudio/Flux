#ifndef TOP_BAR_HPP
#define TOP_BAR_HPP

#include "interface/core/component.hpp"
#include "interface/core/theme.hpp"
#include "interface/widgets/button.hpp"
#include "interface/core/layout.hpp"
#include "engine/dsp/flux_audio_utils.hpp"
#include <string>
#include <functional>
#include <vector>

namespace Beam {

class BeamHost;

class TopBar : public Component {
public:
    TopBar(BeamHost* host) : m_host(host) {
        setName("TopBar");
        
        setupButtons();
        setBounds(0, 0, 1024.0f, 40);
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

        m_fluxBtn->onClick([this]() { setDAWMode(0); });
        m_sliceBtn->onClick([this]() { setDAWMode(1); });
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

    void setDAWMode(int mode) {
        if (m_mode == mode) return;
        m_mode = mode;
        if (m_host && m_host->getMode() != (mode == 0 ? DAWMode::Flux : DAWMode::Splicing)) {
            m_host->setMode(mode == 0 ? DAWMode::Flux : DAWMode::Splicing);
        }
        if (onModeChanged) onModeChanged(mode);
        updateButtonStates();
        resized(); // Re-layout to show/hide tools
    }

    void updateButtonStates() {
        m_fluxBtn->setToggleState(m_mode == 0, false);
        m_sliceBtn->setToggleState(m_mode == 1, false);
        m_playBtn->setToggleState(m_isPlaying, false);
        m_pauseBtn->setToggleState(!m_isPlaying, false);
        
        bool sliceMode = (m_mode == 1);
        m_pointerBtn->setVisible(sliceMode);
        m_scissorsBtn->setVisible(sliceMode);
        m_glueBtn->setVisible(sliceMode);
    }

    void resized() override {
        FlexBox box;
        box.flexDirection(FlexBox::Direction::Row);
        box.alignItems(FlexBox::AlignItems::Center);
        box.justifyContent(FlexBox::JustifyContent::FlexStart);

        auto getWidth = [](std::shared_ptr<Button> b) {
            return AudioUtils::calculateTextWidth(b->getButtonText(), 12.0f) + 20.0f;
        };

        float h = 24.0f;
        float margin = 5.0f;

        // Left Section: Modes
        box.addItem(LayoutItem(m_fluxBtn.get()).withFixedSize(getWidth(m_fluxBtn), h).withMargin(margin));
        box.addItem(LayoutItem(m_sliceBtn.get()).withFixedSize(getWidth(m_sliceBtn), h).withMargin(margin));
        box.addItem(LayoutItem(m_configBtn.get()).withFixedSize(getWidth(m_configBtn), h).withMargin(margin));

        // Tools (Only in Slice Mode)
        if (m_mode == 1) {
            box.addItem(LayoutItem().withFixedSize(20, 0)); // Spacer
            box.addItem(LayoutItem(m_pointerBtn.get()).withFixedSize(getWidth(m_pointerBtn), h).withMargin(margin));
            box.addItem(LayoutItem(m_scissorsBtn.get()).withFixedSize(getWidth(m_scissorsBtn), h).withMargin(margin));
            box.addItem(LayoutItem(m_glueBtn.get()).withFixedSize(getWidth(m_glueBtn), h).withMargin(margin));
        } else {
             m_pointerBtn->setBounds(-100, -100, 0, 0);
             m_scissorsBtn->setBounds(-100, -100, 0, 0);
             m_glueBtn->setBounds(-100, -100, 0, 0);
        }

        box.addItem(LayoutItem().withFlex(1.0f)); 
        
        box.addItem(LayoutItem(m_rewindBtn.get()).withFixedSize(40, h).withMargin(2));
        box.addItem(LayoutItem(m_playBtn.get()).withFixedSize(40, h).withMargin(2));
        box.addItem(LayoutItem(m_pauseBtn.get()).withFixedSize(40, h).withMargin(2));
        box.addItem(LayoutItem(m_recordBtn.get()).withFixedSize(40, h).withMargin(2));

        box.addItem(LayoutItem().withFlex(1.0f));

        // Right Section: Project
        box.addItem(LayoutItem(m_renderBtn.get()).withFixedSize(getWidth(m_renderBtn), h).withMargin(margin));
        box.addItem(LayoutItem(m_loadBtn.get()).withFixedSize(getWidth(m_loadBtn), h).withMargin(margin));
        box.addItem(LayoutItem(m_saveBtn.get()).withFixedSize(getWidth(m_saveBtn), h).withMargin(margin));

        box.performLayout({0, 0, m_bounds.w, m_bounds.h});
    }

    void paint(QuadBatcher& batcher) override {
        // "Console Unit" Look
        batcher.drawChassisPanel(0, 0, m_bounds.w, m_bounds.h, 0, Theme::Console.r, Theme::Console.g, Theme::Console.b, 1.0f);
        
        // Green LED Strip at bottom
        batcher.drawRoundedRect(0, m_bounds.h - 4, m_bounds.w, 4, 2.0f, 0.5f, Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b, 0.8f);
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
    BeamHost* m_host;
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