#ifndef PLACEHOLDER_NODE_HPP
#define PLACEHOLDER_NODE_HPP

#include "engine/core/flux_node.hpp"
#include "interface/core/component.hpp"
#include "interface/widgets/label.hpp"
#include "interface/core/theme.hpp"
#include <string>

namespace Beam {

/**
 * @class PlaceholderProcessor
 * @brief A passthrough processor that keeps the signal flowing when a plugin is missing.
 */
class PlaceholderProcessor : public FluxProcessor {
public:
    void process(const float** inputs, float** outputs, int frames) override {
        // Passthrough: Copy Input 1/2 to Output 1/2
        // If inputs are missing, silence outputs.
        for (int ch = 0; ch < 2; ++ch) {
            if (outputs[ch]) {
                if (inputs[ch]) {
                    // Copy
                    std::copy(inputs[ch], inputs[ch] + frames, outputs[ch]);
                } else {
                    // Silence
                    std::fill(outputs[ch], outputs[ch] + frames, 0.0f);
                }
            }
        }
    }
};

/**
 * @class PlaceholderNode
 * @brief A node that replaces a missing plugin. 
 * It preserves the original state data so that saving the project 
 * doesn't cause data loss for the missing plugin.
 */
class PlaceholderNode : public FluxNode {
public:
    PlaceholderNode(const std::string& originalName, const std::string& originalType, const nlohmann::json& originalState)
        : m_name(originalName), m_originalType(originalType), m_savedState(originalState) 
    {
    }

    std::string getName() const override { return m_name; }

    std::vector<Port> getInputPorts() const override { 
        return { {"In L", 1}, {"In R", 1} }; 
    }

    std::vector<Port> getOutputPorts() const override { 
        return { {"Out L", 1}, {"Out R", 1} }; 
    }

    std::shared_ptr<FluxProcessor> createProcessor() override {
        return std::make_shared<PlaceholderProcessor>();
    }

    std::shared_ptr<Component> createEditor(const NodeEditorContext& ctx) override {
        auto container = std::make_shared<Component>();
        container->setName("PlaceholderEditor");
        
        auto label = std::make_shared<Label>("MISSING PLUGIN:\n" + getName());
        label->setColor(Theme::Red);
        label->setBounds(10, 10, 180, 80);
        
        container->addChildComponent(label);
        return container;
    }

    // Override serialize to write back the ORIGINAL state
    nlohmann::json serialize() const override {
        nlohmann::json data = m_savedState;
        data["type"] = m_originalType; 
        data["name"] = getName();
        return data;
    }

private:
    std::string m_name;
    std::string m_originalType;
    nlohmann::json m_savedState;
};

} // namespace Beam

#endif // PLACEHOLDER_NODE_HPP
