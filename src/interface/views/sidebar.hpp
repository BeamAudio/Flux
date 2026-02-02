#ifndef SIDEBAR_HPP
#define SIDEBAR_HPP

#include "interface/core/component.hpp"
#include "interface/widgets/button.hpp"
#include "interface/widgets/label.hpp"
#include "interface/core/layout.hpp"
#include "interface/core/theme.hpp"
#include "engine/dsp/flux_audio_utils.hpp"
#include <functional>
#include <string>
#include <vector>

namespace Beam {

class BeamHost;

class Sidebar : public Component {
public:
    enum class Side { Left, Right };
    enum class Mode { Browser, Inspector };

    Sidebar(BeamHost* host, Side side) : m_host(host), m_side(side), m_category("NONE") {
        setName("Sidebar");
        rebuildUI();
    }

    void setCategory(const std::string& cat) {
        m_category = cat;
        rebuildUI();
        if (getParent()) getParent()->resized();
        resized();
    }

    void setMode(Mode mode) {
        if (m_mode == mode) return;
        m_mode = mode;
        rebuildUI();
    }

    void rebuildUI() {
        m_children.clear(); // Clear components
        m_buttons.clear();
        m_backBtn = nullptr;

        if (m_side != Side::Left) return;

        if (m_mode == Mode::Inspector) {
            // --- Inspector Mode ---
            auto title = std::make_shared<Label>("INSPECTOR");
            title->setFontSize(20);
             // Emerald Green Title
            title->setColor({0.13f, 0.62f, 0.42f, 1.0f}); 
            addChildComponent(title);
            // We store it in m_buttons just for layout convenience for now, or added to children? 
            // Layout is handled in resized(), which iterates m_buttons... 
            // We need to adapt resized() to handle arbitrary children or keep using specific lists.
            // For now, let's keep using m_buttons as generic "list items" or create a separate list.
            // Let's modify resized() to just layout *all* children vertically? 
            // No, Header vs Scrollable content.
            // Let's just add labels to m_children and handle layout manually in resized if we want simple stack.
        } 
        else {
            // --- Browser Mode ---
            std::string titleText = (m_category == "NONE") ? "LIBRARY" : "< " + m_category;
            m_backBtn = std::make_shared<TextButton>(titleText);
            m_backBtn->onClick([this]() {
                if (m_category != "NONE") setCategory("NONE");
            });
            addChildComponent(m_backBtn);

            std::vector<std::string> items;
            if (m_category == "NONE") {
                items = {"EQUALIZERS", "DYNAMICS", "SPACE", "TIME", "UTILITY"};
            } else if (m_category == "EQUALIZERS") {
                items = {"Tube-P EQ", "Console-E", "Vintage-G", "Graphic-10", "Air-Lift"};
            } else if (m_category == "DYNAMICS") {
                items = {"Opto-2A", "FET-76", "VCA-Bus", "Vari-Mu", "Tube Limiter"};
            } else if (m_category == "SPACE") {
                items = {"Steel Plate", "Golden Hall", "Copper Spring", "Cathedral", "Grain Verb"};
            } else if (m_category == "TIME") {
                items = {"Echo-Plex", "BBD-Bucket", "Reverse", "Ping-Pong", "Space Shift"};
            } else if (m_category == "UTILITY") {
                items = {"Gain", "Filter", "Empty Tape", "Audio Input", "MIDI Input", "Spectrum", "Loudness"};
            }

            for (const auto& item : items) {
                auto btn = std::make_shared<TextButton>(item);
                if (m_category == "NONE") {
                    btn->onClick([this, item]() { setCategory(item); });
                } else {
                    btn->onClick([this, item]() { if (onAddFX) onAddFX(item); });
                }
                addChildComponent(btn);
                m_buttons.push_back(btn);
            }
        }
        resized();
    }

    void resized() override {
        if (m_side != Side::Left) return;

        FlexBox box;
        box.flexDirection(FlexBox::Direction::Column);
        box.alignItems(FlexBox::AlignItems::Stretch);
        box.padding(15); // Increased padding

        if (m_mode == Mode::Inspector) {
             for (auto& child : m_children) {
                 box.addItem(LayoutItem(child.get()).withHeight(30).withMargin(5));
             }
        } else {
            // Browser Layout
            if (m_backBtn) {
                // Back Button as a header
                box.addItem(LayoutItem(m_backBtn.get()).withHeight(35).withMargin(10));
            }
            for (auto& btn : m_buttons) {
                // Module-like items
                box.addItem(LayoutItem(btn.get()).withHeight(32).withMargin(4));
            }
        }
        
        Rect area = {0, 0, m_bounds.w, m_bounds.h};
        box.performLayout(area);
    }

    void paint(QuadBatcher& batcher) override {
        // Gradient Background (Dark Bakelite to slightly lighter)
        batcher.drawGradientRect(0, 0, m_bounds.w, m_bounds.h, 
                                Theme::Bakelite.r, Theme::Bakelite.g, Theme::Bakelite.b, 1.0f,
                                Theme::Bakelite.brighter(0.1f).r, Theme::Bakelite.brighter(0.1f).g, Theme::Bakelite.brighter(0.1f).b, 1.0f);
        
        // Inner shadow/bezel on the right
        if (m_side == Side::Left) {
             batcher.drawQuad(m_bounds.w - 4, 0, 4, m_bounds.h, 0.0f, 0.0f, 0.0f, 0.3f);
             batcher.drawQuad(m_bounds.w - 1, 0, 1, m_bounds.h, Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b, 0.4f);
        }
        
        // Header Area Background
        batcher.drawQuad(0, 0, m_bounds.w, 50, 0.0f, 0.0f, 0.0f, 0.2f);
    }

    std::function<void(std::string)> onAddFX;

private:
    BeamHost* m_host;
    Side m_side;
    Mode m_mode = Mode::Browser;
    std::string m_category;
    std::shared_ptr<TextButton> m_backBtn;
    std::vector<std::shared_ptr<TextButton>> m_buttons;
};

} // namespace Beam

#endif // SIDEBAR_HPP





