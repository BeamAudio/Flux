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

        m_saveBtn = std::make_shared<TextButton>("SAVE");
        addChildComponent(m_saveBtn);
        m_saveBtn->onClick([this]() { if (onSave) onSave(); });

        m_discardBtn = std::make_shared<TextButton>("DISCARD");
        addChildComponent(m_discardBtn);
        m_discardBtn->onClick([this]() { if (onDiscard) onDiscard(); });

        m_cancelBtn = std::make_shared<TextButton>("CANCEL");
        addChildComponent(m_cancelBtn);
        m_cancelBtn->onClick([this]() { if (onCancel) onCancel(); });

        // Set bounds AFTER buttons are created to avoid null issues
        setBounds(0, 0, 420, 160);
    }

    void paint(QuadBatcher& batcher) override {
        // Window background
        batcher.drawRoundedRect(0.0f, 0.0f, m_bounds.w, m_bounds.h, 6.0f, 0.5f, 0.15f, 0.15f, 0.18f, 1.0f);
        // Title bar
        batcher.drawQuad(0.0f, 0.0f, m_bounds.w, 30.0f, 0.12f, 0.12f, 0.15f, 1.0f);
        batcher.drawText("Unsaved Changes", 12.0f, 8.0f, 14.0f, 0.9f, 0.9f, 0.9f, 1.0f);

        // Message
        batcher.drawText(m_message, 20, 55, 14.0f, 0.85f, 0.85f, 0.85f, 1.0f);
    }

    void resized() override {
        // Guard against being called before buttons are created
        if (!m_saveBtn || !m_discardBtn || !m_cancelBtn) return;
        
        float btnW = 90.0f;
        float btnH = 28.0f;
        float spacing = 10.0f;
        float margin = 15.0f;
        
        // Calculate total width of buttons + spacing
        float totalBtnWidth = (btnW * 3) + (spacing * 2);
        float startX = m_bounds.w - totalBtnWidth - margin;
        float y = m_bounds.h - btnH - margin;

        m_saveBtn->setBounds(startX, y, btnW, btnH);
        m_discardBtn->setBounds(startX + btnW + spacing, y, btnW, btnH);
        m_cancelBtn->setBounds(startX + (btnW + spacing) * 2, y, btnW, btnH);
    }

    std::function<void()> onSave;
    std::function<void()> onDiscard;
    std::function<void()> onCancel;

private:
    std::string m_message;
    std::shared_ptr<TextButton> m_saveBtn;
    std::shared_ptr<TextButton> m_discardBtn;
    std::shared_ptr<TextButton> m_cancelBtn;
};

} // namespace Beam

#endif

