#ifndef SIDEBAR_HPP
#define SIDEBAR_HPP

#include "interface/core/component.hpp"
#include "interface/widgets/button.hpp"
#include "interface/widgets/label.hpp"
#include "interface/core/layout.hpp"
#include "interface/core/theme.hpp"
#include "interface/core/scrollable_container.hpp"
#include "engine/dsp/flux_audio_utils.hpp"
#include <functional>
#include <string>
#include <vector>

#include "engine/plugins/plugin_registry.hpp"

namespace Beam {

class BeamHost;

class Sidebar : public Component {
public:
    enum class Side { Left, Right };
    enum class Mode { Browser, Inspector };

    Sidebar(BeamHost* host, Side side) : m_host(host), m_side(side), m_category("NONE") {
        setName("Sidebar");
        
        m_scrollContainer = std::make_shared<ScrollableContainer>();
        m_contentPane = std::make_shared<Component>();
        m_contentPane->setName("SidebarContent");
        m_scrollContainer->setContent(m_contentPane);
        addChildComponent(m_scrollContainer);

        rebuildUI();

        PluginRegistry::get().onRegistryChanged = [this]() {
            m_needsRebuild = true; 
        };
    }

    void update(float dt) override {
        if (m_needsRebuild) {
            m_needsRebuild = false;
            rebuildUI();
            if (getParent()) getParent()->resized();
        }
        Component::update(dt);
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
        m_children.clear(); 
        m_buttons.clear();
        m_backBtn = nullptr;

        if (m_contentPane) m_contentPane->clearChildren();
        if (m_scrollContainer) addChildComponent(m_scrollContainer);

        if (m_side != Side::Left) return;

        if (m_mode == Mode::Inspector) {
            auto title = std::make_shared<Label>("INSPECTOR");
            title->setFontSize(20);
            title->setColor({0.13f, 0.62f, 0.42f, 1.0f}); 
            m_contentPane->addChildComponent(title);
        } 
        else {
            std::string titleText = (m_category == "NONE") ? "LIBRARY" : "< " + m_category;
            m_backBtn = std::make_shared<TextButton>(titleText);
            m_backBtn->onClick([this]() {
                if (m_category != "NONE") setCategory("NONE");
            });
            m_contentPane->addChildComponent(m_backBtn);

            const auto& allPlugins = PluginRegistry::get().getAvailablePlugins();
            std::vector<std::string> items;
            
            if (m_category == "NONE") {
                std::vector<std::string> cats;
                for (const auto& name : allPlugins) {
                    std::string c = PluginRegistry::get().getPluginCategory(name);
                    bool found = false;
                    for (const auto& existing : cats) if(existing == c) { found = true; break; }
                    if (!found) cats.push_back(c);
                }
                items = cats;
                std::sort(items.begin(), items.end());
            } else {
                for (const auto& name : allPlugins) {
                    if (PluginRegistry::get().getPluginCategory(name) == m_category) {
                        items.push_back(name);
                    }
                }
                std::sort(items.begin(), items.end());
            }

            for (const auto& item : items) {
                auto btn = std::make_shared<TextButton>(item);
                if (m_category == "NONE") {
                    btn->onClick([this, item]() { setCategory(item); });
                } else {
                    btn->onClick([this, item]() { if (onAddFX) onAddFX(item); });
                }
                m_contentPane->addChildComponent(btn);
                m_buttons.push_back(btn);
            }
        }
        resized();
    }

    void resized() override {
        if (m_side != Side::Left) return;

        m_scrollContainer->setBounds(0, 50, m_bounds.w, m_bounds.h - 50);

        FlexBox box;
        box.flexDirection(FlexBox::Direction::Column);
        box.alignItems(FlexBox::AlignItems::Stretch);
        box.padding(15); 

        if (m_mode == Mode::Inspector) {
             for (auto& child : m_contentPane->getChildren()) {
                 box.addItem(LayoutItem(child.get()).withHeight(30).withMargin(5));
             }
        } else {
            if (m_backBtn) {
                box.addItem(LayoutItem(m_backBtn.get()).withHeight(35).withMargin(10));
            }
            for (auto& btn : m_buttons) {
                box.addItem(LayoutItem(btn.get()).withHeight(32).withMargin(4));
            }
        }
        
        float ph = box.calculateHeight(m_bounds.w);
        m_contentPane->setBounds(0, 0, m_bounds.w, ph);
        box.performLayout({0, 0, m_bounds.w, ph});
        
        m_scrollContainer->updateContentBounds();
    }

    void paint(QuadBatcher& batcher) override {
        // ... (existing paint code) ...
        batcher.drawGradientRect(0, 0, m_bounds.w, m_bounds.h, 
                                Theme::Bakelite.r, Theme::Bakelite.g, Theme::Bakelite.b, 1.0f,
                                Theme::Bakelite.brighter(0.1f).r, Theme::Bakelite.brighter(0.1f).g, Theme::Bakelite.brighter(0.1f).b, 1.0f);
        
        if (m_side == Side::Left) {
             batcher.drawQuad(m_bounds.w - 4, 0, 4, m_bounds.h, 0.0f, 0.0f, 0.0f, 0.3f);
             batcher.drawQuad(m_bounds.w - 1, 0, 1, m_bounds.h, Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b, 0.4f);
        }
        
        batcher.drawQuad(0, 0, m_bounds.w, 50, 0.0f, 0.0f, 0.0f, 0.2f);
    }

    bool onMouseWheel(float x, float y, float delta, bool shift) override {
        if (m_scrollContainer) return m_scrollContainer->onMouseWheel(x, y, delta, shift);
        return false;
    }

    std::function<void(std::string)> onAddFX;

private:
    BeamHost* m_host;
    Side m_side;
    Mode m_mode = Mode::Browser;
    std::string m_category;
    bool m_needsRebuild = false;
    std::shared_ptr<ScrollableContainer> m_scrollContainer;
    std::shared_ptr<Component> m_contentPane;
    std::shared_ptr<TextButton> m_backBtn;
    std::vector<std::shared_ptr<TextButton>> m_buttons;
};

} // namespace Beam

#endif // SIDEBAR_HPP





