#include "engine/core/flux_node.hpp"
#include "interface/editors/generic_node_editor.hpp"

namespace Beam {

std::shared_ptr<Component> FluxNode::createEditor(const NodeEditorContext& context) {
    // Default to Generic Editor
    return std::make_shared<GenericNodeEditor>(this);
}

} // namespace Beam
