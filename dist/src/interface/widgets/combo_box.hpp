#ifndef COMBO_BOX_HPP
#define COMBO_BOX_HPP

#include "interface/core/component.hpp"
#include "interface/core/theme.hpp"
#include "interface/widgets/button.hpp"
#include "interface/popup/popup_host.hpp"
#include <string>
#include <vector>
#include <functional>

namespace Beam {

class PopupMenu : public Component {
public:
    PopupMenu(const std::vector<std::string>& items, std::function<void(int)> onSelect) 
        : m_items(items), m_onSelect(onSelect) {
        setName("PopupMenu");
    }

    void resized() override {
    }

    void paint(QuadBatcher& g) override {
        // Dropdown Menu Background
        g.drawBeveledRect(0, 0, m_bounds.w, m_bounds.h, 4.0f, 1.0f, Theme::Bakelite.r, Theme::Bakelite.g, Theme::Bakelite.b, 1.0f);
        g.drawRoundedRect(0, 0, m_bounds.w, m_bounds.h, 4.0f, 1.0f, Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b, 0.3f); // Glow

        float itemH = 20.0f;
        for (size_t i = 0; i < m_items.size(); ++i) {
            float y = i * itemH;
            if (m_hoverIndex == (int)i) {
                g.drawQuad(2, y, m_bounds.w - 4, itemH, Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b, 0.6f);
            }
            g.drawText(m_items[i], 5, y + 4, 11, 0.9f, 0.9f, 0.9f, 1.0f);
        }
    }

    bool onMouseMove(float x, float y, bool shift) override {
        float itemH = 20.0f;
        m_hoverIndex = (int)(y / itemH);
        return true;
    }

    bool onMouseDown(float x, float y, int button, bool shift) override {
        if (m_hoverIndex >= 0 && m_hoverIndex < (int)m_items.size()) {
            if (m_onSelect) m_onSelect(m_hoverIndex);
            return true;
        }
        return false;
    }

private:
    std::vector<std::string> m_items;
    std::function<void(int)> m_onSelect;
    int m_hoverIndex = -1;
};

class ComboBox : public Component {
public:
    ComboBox() {
        setName("ComboBox");
        m_bg = std::make_shared<Button>("");
        addChildComponent(m_bg);
        
        m_bg->onClick([this]() { showPopup(); });
    }

    void addItem(const std::string& item) {
        m_items.push_back(item);
    }

    void clear() { m_items.clear(); m_selected = -1; }

    void setSelectedId(int index) {
        if (index >= 0 && index < (int)m_items.size()) {
            m_selected = index;
            m_bg->setButtonText(m_items[index]);
        } else {
            m_bg->setButtonText("");
        }
    }

    int getSelectedId() const { return m_selected; }

    void setOnChange(std::function<void(int)> callback) { m_onChange = callback; }

    void getPreferredSize(float& w, float& h) const override {
        w = 150.0f;
        h = 24.0f;
    }

    void resized() override {
        m_bg->setBounds(0, 0, m_bounds.w, m_bounds.h);
    }

    void showPopup() {
        auto host = findParent<PopupHost>();
        if (host) {
            auto popup = std::make_shared<PopupMenu>(m_items, [this, host](int index) {
                setSelectedId(index);
                if (m_onChange) m_onChange(index);
                host->closePopup();
            });
            
            Rect r = getScreenBounds(); 
            popup->setBounds(r.x, r.y + r.h, r.w, (float)m_items.size() * 24.0f + 10.0f);
            host->showPopup(popup);
        }
    }

private:
    std::shared_ptr<Button> m_bg;
    std::vector<std::string> m_items;
    int m_selected = -1;
    std::function<void(int)> m_onChange;
};

} // namespace Beam

#endif // COMBO_BOX_HPP
