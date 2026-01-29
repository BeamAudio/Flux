#ifndef FILTER_MODULE_HPP
#define FILTER_MODULE_HPP

#include "audio_module.hpp"
#include "filter_graph.hpp"
#include "../engine/flux_fx_nodes.hpp"

namespace Beam {

class FilterModule : public AudioModule {
public:
    FilterModule(std::shared_ptr<FluxFilterNode> node, size_t nodeId, float x, float y) 
        : AudioModule(node, nodeId, x, y), m_filterNode(node) {
        
        m_graph = std::make_shared<FilterGraph>(node->getInternalFilter());
        addChild(m_graph);
        
        // Resize to fit the graph
        setBounds(x, y, 160, 260);
    }

    void setBounds(float x, float y, float w, float h) override {
        AudioModule::setBounds(x, y, w, h);
        if (m_graph) {
            // Position graph below the title bar
            m_graph->setBounds(x + 15, y + 45, w - 30, 80);
            
            // Re-position knobs below the graph
            float startY = 135;
            float spacing = 55;
            int count = 0;
            for (auto& child : m_children) {
                if (child != m_graph) {
                    child->setBounds(x + 25, y + startY + (count * spacing), 110, 40);
                    count++;
                }
            }
        }
    }

private:
    std::shared_ptr<FluxFilterNode> m_filterNode;
    std::shared_ptr<FilterGraph> m_graph;
};

} // namespace Beam

#endif // FILTER_MODULE_HPP



