#include "interface/editors/filter_editor.hpp"
#include "interface/editors/filter_graph.hpp"
#include "interface/widgets/slider.hpp"
#include "interface/widgets/label.hpp"
#include "interface/core/layout.hpp"
#include "engine/nodes/flux_fx_nodes.hpp"

namespace Beam {

FilterEditor::FilterEditor(FluxFilterNode* node) {
    if (!node) return;
    
    // Graph
    m_graph = std::make_shared<FilterGraph>(node->getInternalFilter());
    addChildComponent(m_graph);
    
    m_layout.flexDirection(FlexBox::Direction::Column);
    m_layout.alignItems(FlexBox::AlignItems::Stretch);

    // Add Graph Item
    m_layout.addItem(LayoutItem(m_graph.get()).withFixedSize(0, 80).withMargin(5));

    // Params
    for(auto const& [name, param] : node->getParameters()) {
        auto lbl = std::make_shared<Label>(name);
        auto sld = std::make_shared<Slider>();
        sld->setParameter(param);
        
        m_children.push_back(lbl);
        m_children.push_back(sld);
        addChildComponent(lbl);
        addChildComponent(sld);

        m_layout.addItem(LayoutItem(lbl.get()).withFixedSize(0, 15).withMargin(2));
        m_layout.addItem(LayoutItem(sld.get()).withFixedSize(0, 25).withMargin(2));
    }
}

void FilterEditor::resized() {
    m_layout.performLayout({0, 0, m_bounds.w, m_bounds.h});
}

} // namespace Beam
