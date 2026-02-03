#ifndef CONFIRMATION_MODAL_HPP
#define CONFIRMATION_MODAL_HPP

#include "interface/core/component.hpp"
#include "interface/core/theme.hpp"
#include "interface/widgets/button.hpp"
#include <string>
#include <functional>

namespace Beam {

class ConfirmationModal : public Component {
public:
    ConfirmationModal(const std::string& message) : m_message(message) {
        setName("ConfirmationModal");
        setBounds(0, 0, 400, 160);

        m_saveBtn = std::make_shared<TextButton>("SAVE");
        addChildComponent(m_saveBtn);
        m_saveBtn->onClick([this]() { if (onSave) onSave(); });

        m_discardBtn = std::make_shared<TextButton>("DISCARD");
        addChildComponent(m_discardBtn);
        m_discardBtn->onClick([this]() { if (onDiscard) onDiscard(); });

        m_cancelBtn = std::make_shared<TextButton>("CANCEL");
        addChildComponent(m_cancelBtn);
        m_cancelBtn->onClick([this]() { if (onCancel) onCancel(); });
    }

    void paint(QuadBatcher& batcher) override {
        // Window
        batcher.drawRoundedRect(0.0f, 0.0f, m_bounds.w, m_bounds.h, 6.0f, 0.5f, 0.15f, 0.15f, 0.18f, 1.0f);
        batcher.drawQuad(0.0f, 0.0f, m_bounds.w, 30.0f, 0.12f, 0.12f, 0.15f, 1.0f); // Title bar
        batcher.drawText("Unsaved Changes", 12.0f, 8.0f, 14.0f, 0.9f, 0.9f, 0.9f, 1.0f);

        // Message
        batcher.drawText(m_message, 20, 60, 16.0f, 0.85f, 0.85f, 0.85f, 1.0f);
    }

    void resized() override {
        float btnW = 100.0f;
        float btnH = 28.0f;
        float spacing = 10.0f;
        float startX = m_bounds.w - (btnW * 3 + spacing * 3);
        float y = m_bounds.h - 40.0f;

        m_saveBtn->setBounds(startX, y, btnW, btnH);
        m_discardBtn->setBounds(startX + btnW + spacing, y, btnW, btnH);
        m_cancelBtn->setBounds(startX + (btnW + spacing) * 2, y, btnW, btnH);
    }

    std::function<void()> onSave;
    // User wants to proceed and LOSE progress
    std::function<void()> onDiscard;
    // Stop everything
    std::function<void()> onCancel;

private:
    std::string m_message;
    std::shared_ptr<TextButton> m_saveBtn;
    std::shared_ptr<TextButton> m_discardBtn;
    std::shared_ptr<TextButton> m_cancelBtn;
};

} // namespace Beam

#endif
