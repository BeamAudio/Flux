#ifndef SIDEBAR_HPP
#define SIDEBAR_HPP

#include "interface/core/component.hpp"
#include "interface/widgets/button.hpp"
#include "interface/widgets/label.hpp"
#include "interface/core/theme.hpp"
#include "interface/core/scrollable_container.hpp"
#include "engine/plugins/plugin_registry.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <memory>

namespace Beam {

/**
 * @class Sidebar
 * @brief A high-reliability Sidebar implementation for Beam Audio Flux.
 * Uses manual vertical layout and dynamic height calculation to ensure 
 * visual stability and correct interaction handling.
 */
class Sidebar : public Component {
public:
    enum class Side { Left, Right };
    enum class Mode { Browser, Inspector };

    Sidebar(class BeamHost* host, Side side) : m_side(side), m_category("NONE") {
        setName("Sidebar");
        
        // 1. Scrollable List Area
        m_scroll = std::make_shared<ScrollableContainer>();
        m_scroll->setName("SidebarScroll");
        addChildComponent(m_scroll);
        
        m_content = std::make_shared<ContentPane>();
        m_content->setName("SidebarContent");
        m_scroll->setContent(m_content);

        // 2. Header Area (Added last to be on top of list if they overlap)
        m_header = std::make_shared<Component>();
        m_header->setName("SidebarHeader");
        addChildComponent(m_header);

        m_title = std::make_shared<Label>("PLUGIN BROWSER");
        m_title->setFontSize(12);
        m_title->setColor(Theme::White);
        m_header->addChildComponent(m_title);

        rebuildUI();

        PluginRegistry::get().onRegistryChanged = [this]() { m_needsRebuild = true; };
    }

    void update(float dt) override {
        if (m_needsRebuild) { m_needsRebuild = false; rebuildUI(); }
        Component::update(dt);
    }

    void setCategory(const std::string& cat) {
        m_category = cat;
        if (m_scroll) m_scroll->scrollToTop();
        rebuildUI();
    }

    void setMode(Mode mode) {
        if (m_mode == mode) return;
        m_mode = mode;
        m_category = "NONE";
        if (m_scroll) m_scroll->scrollToTop();
        rebuildUI();
    }

    void rebuildUI() {
        m_content->clearChildren();
        
        if (m_mode == Mode::Inspector) {
            m_title->setText("INSPECTOR");
            auto msg = std::make_shared<Label>("NO SELECTION");
            msg->setFontSize(10);
            msg->setColor({0.4f, 0.4f, 0.4f, 1.0f});
            m_content->addChildComponent(msg);
        } else {
            m_title->setText(m_category == "NONE" ? "PLUGIN BROWSER" : m_category);
            
            if (m_category != "NONE") {
                auto back = std::make_shared<TextButton>("< BACK TO LIBRARY");
                back->onClick([this]() { setCategory("NONE"); });
                m_content->addChildComponent(back);
            }

            const auto& allPlugins = PluginRegistry::get().getAvailablePlugins();
            std::vector<std::string> items;
            if (m_category == "NONE") {
                for (const auto& name : allPlugins) {
                    std::string c = PluginRegistry::get().getPluginCategory(name);
                    if (std::find(items.begin(), items.end(), c) == items.end()) items.push_back(c);
                }
            } else {
                for (const auto& name : allPlugins) {
                    if (PluginRegistry::get().getPluginCategory(name) == m_category) items.push_back(name);
                }
            }
            std::sort(items.begin(), items.end());

            for (const auto& name : items) {
                auto btn = std::make_shared<TextButton>(name);
                if (m_category == "NONE") {
                    btn->onClick([this, name]() { setCategory(name); });
                } else {
                    btn->onClick([this, name]() { if (onAddFX) onAddFX(name); });
                }
                m_content->addChildComponent(btn);
            }
        }
        resized();
    }

    void resized() override {
        float hH = 42.0f;
        m_header->setBounds(0, 0, m_bounds.w, hH);
        m_title->setBounds(15, 13, m_bounds.w - 30, 16);

        m_scroll->setBounds(0, hH, m_bounds.w, m_bounds.h - hH);

        // Manual Vertical Layout
        float y = 8.0f;
        float itemH = 26.0f;
        float gap = 2.0f;

        for (auto& child : m_content->getChildren()) {
            if (std::dynamic_pointer_cast<Label>(child)) {
                child->setBounds(15, y, m_bounds.w - 30, 16);
                y += 16 + gap;
            } else {
                child->setBounds(5, y, m_bounds.w - 15, itemH);
                y += itemH + gap;
            }
        }
        
        // Update content bounds and trigger scroll recalculation
        m_scroll->updateContentBounds();
    }

    void paint(QuadBatcher& g) override {
        // Main Background - Darker
        g.drawQuad(0, 0, m_bounds.w, m_bounds.h, Theme::Black.r, Theme::Black.g, Theme::Black.b, 1.0f);
        
        // Header Visual Finish - Darker
        g.drawChassisPanel(0, 0, m_bounds.w, 42, 0.0f, Theme::GreyDark.r, Theme::GreyDark.g, Theme::GreyDark.b, 1.0f);
        g.drawQuad(0, 41, m_bounds.w, 1, Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b, 0.25f);
        
        if (m_side == Side::Left) {
            // Right-side vertical separator (Sharp Line)
            g.drawQuad(m_bounds.w - 1, 0, 1, m_bounds.h, Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b, 0.12f);
        }
    }

    std::function<void(std::string)> onAddFX;

private:
    /**
     * @class ContentPane
     * @brief A specialized component that calculates its own preferred height 
     * based on its children's positions.
     */
    class ContentPane : public Component {
    public:
        void getPreferredSize(float& w, float& h) const override {
            w = m_bounds.w;
            float maxY = 0;
            for (const auto& child : m_children) {
                float bottom = child->getY() + child->getHeight();
                if (bottom > maxY) maxY = bottom;
            }
            h = maxY + 12.0f; 
        }
    };

    Side m_side;
    Mode m_mode = Mode::Browser;
    std::string m_category;
    bool m_needsRebuild = false;
    
    std::shared_ptr<Component> m_header;
    std::shared_ptr<Label> m_title;
    std::shared_ptr<ScrollableContainer> m_scroll;
    std::shared_ptr<Component> m_content;
};

} // namespace Beam

#endif
