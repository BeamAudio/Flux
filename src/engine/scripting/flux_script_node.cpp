#include "engine/scripting/flux_script_node.hpp"
#include "interface/widgets/button.hpp"
#include "interface/core/layout.hpp"
#include "interface/analog/analog_ui_templates.hpp"
#include <iostream>

namespace Beam {

/**
 * @class FluxScriptEditor
 * @brief Custom editor for Script nodes. Wraps RackUnitUI and adds AOT controls.
 */
class FluxScriptEditor : public Component {
public:
    FluxScriptEditor(FluxScriptNode* node) {
        // 1. Rack UI (Handles all parameters and styling)
        m_rackUI = std::make_shared<RackUnitUI>(node, RackUnitUI::Script(node->getName()));
        addChildComponent(m_rackUI);

        // 2. AOT Compile Button
        m_compileBtn = std::make_shared<TextButton>("COMPILE AOT");
        m_compileBtn->onClick([node]() {
            std::cout << "Compiling script to native plugin..." << std::endl;
            if (node->compileToNative("MyCompiledPlugin")) {
                std::cout << "Compilation Success! Plugin 'MyCompiledPlugin' created." << std::endl;
            } else {
                std::cout << "Compilation Failed. Check if 'cl.exe' is in PATH." << std::endl;
            }
        });
        addChildComponent(m_compileBtn);
    }

    void getPreferredSize(float& w, float& h) const override {
        m_rackUI->getPreferredSize(w, h);
        h += 35.0f; // Extra room for button
    }

    void resized() override {
        float rackW = 0, rackH = 0;
        m_rackUI->getPreferredSize(rackW, rackH);
        
        m_rackUI->setBounds(0, 0, m_bounds.w, rackH);
        m_compileBtn->setBounds(10, rackH + 5, m_bounds.w - 20, 24);
    }

private:
    std::shared_ptr<RackUnitUI> m_rackUI;
    std::shared_ptr<TextButton> m_compileBtn;
};

std::shared_ptr<Component> FluxScriptNode::createEditor(const NodeEditorContext& context) {
    return std::make_shared<FluxScriptEditor>(this);
}

} // namespace Beam