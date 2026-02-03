#ifndef FILTER_EDITOR_HPP
#define FILTER_EDITOR_HPP

#include "interface/core/component.hpp"
#include "interface/core/layout.hpp"
#include <vector>
#include <memory>

namespace Beam {

class FluxFilterNode;
class FilterGraph;

class FilterEditor : public Component {
public:
    FilterEditor(FluxFilterNode* node);
    void resized() override;

private:
    std::shared_ptr<FilterGraph> m_graph;
    FlexBox m_layout;
    std::vector<std::shared_ptr<Component>> m_children;
};

} // namespace Beam

#endif
