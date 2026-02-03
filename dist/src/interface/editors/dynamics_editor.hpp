#ifndef DYNAMICS_EDITOR_HPP
#define DYNAMICS_EDITOR_HPP

#include "interface/core/component.hpp"
#include "interface/core/theme.hpp"
#include "interface/widgets/slider.hpp"
#include "interface/widgets/label.hpp"
#include "interface/core/layout.hpp"
#include "interface/editors/generic_node_editor.hpp"
#include "engine/plugins/flux_plugin.hpp"

namespace Beam {

class DynamicsEditor : public Component {
public:
    DynamicsEditor(FluxNode* node, std::function<float()> grCallback = nullptr) 
        : m_node(node), m_grCallback(grCallback) {
        
        m_layout.flexDirection(FlexBox::Direction::Column);
        m_layout.alignItems(FlexBox::AlignItems::Stretch);

        // Meter Area
        m_layout.addItem(LayoutItem().withFixedSize(0, 20).withMargin(5));

        // Params
        if (node) {
            for(auto const& [name, param] : node->getParameters()) {
                auto row = std::make_shared<ParameterRow>(name, param);
                addChildComponent(row);
                m_rows.push_back(row);
                m_layout.addItem(LayoutItem(row.get()).withFixedSize(0, 24).withMargin(2));
            }
        }
    }

    void resized() override {
        m_meterBounds = {5, 5, m_bounds.w - 10, 15};
        m_layout.performLayout({0, 0, m_bounds.w, m_bounds.h});
    }

    void paint(QuadBatcher& batcher) override {
        // Draw Meter Background (Local)
        batcher.drawRoundedRect(m_meterBounds.x, m_meterBounds.y, m_meterBounds.w, m_meterBounds.h, 2.0f, 0.5f, 0.05f, 0.05f, 0.05f, 1.0f);
        
        float gr = 0.0f;
        if (m_grCallback) {
            gr = m_grCallback();
        } else if (m_node && m_node->getMeterSource() && m_node->getMeterSource()->getNumMeters() > 0) {
            gr = m_node->getMeterSource()->getValue(0); // Assume first meter is GR
        }

        float grNorm = std::clamp(gr, 0.0f, 1.0f);
        float activeW = m_meterBounds.w * grNorm;
        
        batcher.drawRoundedRect(m_meterBounds.x + m_meterBounds.w - activeW, m_meterBounds.y, activeW, m_meterBounds.h, 2.0f, 0.5f, Theme::Red.r, Theme::Red.g, Theme::Red.b, 1.0f);
        
        // GR Text
        if (gr > 0.01f) {
            char buf[16];
            snprintf(buf, 16, "-%.1f dB", gr * 20.0f); 
            batcher.drawText(buf, m_meterBounds.x + 5, m_meterBounds.y + 3, 10, 1.0f, 1.0f, 1.0f, 0.7f);
        }
    }

    // Force repaint to animate meter
    void render(QuadBatcher& batcher, float dt, float w, float h) override {
        Component::render(batcher, dt, w, h);
    }

private:
    FluxNode* m_node;
    std::function<float()> m_grCallback;
    FlexBox m_layout;
    std::vector<std::shared_ptr<ParameterRow>> m_rows;
    Rect m_meterBounds;
};

} // namespace Beam

#endif
