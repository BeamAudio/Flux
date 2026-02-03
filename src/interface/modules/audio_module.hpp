#ifndef AUDIO_MODULE_HPP
#define AUDIO_MODULE_HPP

#include "interface/core/component.hpp"
#include "interface/modules/port.hpp"
#include "interface/widgets/button.hpp"
#include "interface/core/layout.hpp"
#include "interface/editors/generic_node_editor.hpp"
#include "interface/editors/vst_external_editor.hpp"
#include "engine/core/flux_node.hpp"
#include "engine/core/audio_device_manager.hpp"
#include "engine/dsp/flux_audio_utils.hpp"
#include <string>
#include <vector>
#include <functional>

namespace Beam {

/**
 * @class AudioModule
 * @brief Base UI component for any FluxNode (FX, Instrument, Input, etc.)
 * Features dynamic layout and auto-sizing to prevent overlapping controls.
 */
class AudioModule : public Component {
public:
    AudioModule(std::shared_ptr<FluxNode> node, size_t nodeId, float x, float y, AudioDeviceManager* deviceManager = nullptr, void* nativeWindowHandle = nullptr) 
        : m_node(node), m_nodeId(nodeId), m_deviceManager(deviceManager), m_nativeWindowHandle(nativeWindowHandle) {
        
        setName(node->getName());

        for (int i = 0; i < (int)node->getInputPorts().size(); ++i) {
            auto const& p = node->getInputPorts()[i];
            auto portType = (p.type == FluxNode::Port::Sidechain) ? PortType::Sidechain : PortType::Input;
            auto port = std::make_shared<Port>(portType, this, i);
            m_inputPorts.push_back(port);
            addChildComponent(port);
        }
            
        if (node->getName() != "Master") {
            for (int i = 0; i < (int)node->getOutputPorts().size(); ++i) {
                auto port = std::make_shared<Port>(PortType::Output, this, i);
                m_outputPorts.push_back(port);
                addChildComponent(port);
            }
        }
        
        setDraggable(true);
        setClipsChildren(false); 
        
        setBounds(x, y, 160, 100); 
        autoGenerateUI(); 
    }

    size_t getNodeId() const { return m_nodeId; }

    virtual void autoGenerateUI() {
        if (!m_node) return;
        m_children.clear();
        
        // Restore ports
        for (auto& p : m_inputPorts) addChildComponent(p);
        for (auto& p : m_outputPorts) addChildComponent(p);

        // Create Context
        NodeEditorContext ctx;
        ctx.deviceManager = m_deviceManager;
        ctx.nativeWindowHandle = m_nativeWindowHandle;

        m_editorComponent = m_node->createEditor(ctx);

        if (!m_editorComponent) {
             m_editorComponent = std::make_shared<GenericNodeEditor>(m_node.get());
        }

        if (m_editorComponent) {
            // Adaptive: Allow editor to grow
            m_editorComponent->setClipsChildren(false); 
            addChildComponent(m_editorComponent);
            
            if (auto vstEditor = std::dynamic_pointer_cast<VSTExternalEditor>(m_editorComponent)) {
                vstEditor->attachToNative((HWND)m_nativeWindowHandle);
            }
            
            // Auto-size the editor based on its own preferred content
            m_editorComponent->autoSize(true);
            
            updateModuleSizeToFitEditor();
        }
    }

    void updateModuleSizeToFitEditor() {
        if (!m_editorComponent) return;
        
        float pw = 0, ph = 0;
        m_editorComponent->getPreferredSize(pw, ph);
        
        static constexpr float HEADER_H = 30.0f;
        static constexpr float PADDING = 10.0f;
        
        float totalW = (std::max)(pw + PADDING * 2, 200.0f);
        float totalH = HEADER_H + ph + PADDING * 2;
        
        float portStartY = 50.0f;
        float portSpacing = 20.0f;
        float maxPortY = portStartY + (float)(std::max)(m_inputPorts.size(), m_outputPorts.size()) * portSpacing;
        totalH = (std::max)(totalH, maxPortY + PADDING);

        // Apply new bounds
        setBounds(m_bounds.x, m_bounds.y, totalW, totalH);
    }

    void childBoundsChanged(Component* child) override {
        if (child == m_editorComponent.get()) {
             updateModuleSizeToFitEditor();
        }
        Component::childBoundsChanged(child);
    }
    
    // Override hit test to allow clicking ports outside main bounds
    bool onMouseDown(float x, float y, int button, bool shift) override {
        // Convert to Local Space
        float localX = x - m_bounds.x;
        float localY = y - m_bounds.y;

        // Check Ports first (they might overhang)
        auto checkPorts = [&](const std::vector<std::shared_ptr<Port>>& ports) {
            for (auto& p : ports) {
                if (p && p->getBounds().contains(localX, localY)) {
                    return p->onMouseDown(localX - p->getX(), localY - p->getY(), button, shift);
                }
            }
            return false;
        };
        
        if (checkPorts(m_inputPorts)) return true;
        if (checkPorts(m_outputPorts)) return true;
        
        if (getName() != "Master" && getName() != "MASTER" && m_deleteBtnBounds.contains(localX, localY)) {
            if (onDeleteRequested) onDeleteRequested(this);
            return true;
        }
        
        // Pass original Parent Coords to base, it handles conversion
        return Component::onMouseDown(x, y, button, shift);
    }

    void resized() override {
        static constexpr float HEADER_H = 30.0f;
        static constexpr float PADDING = 8.0f;
        float portStartY = 50.0f;
        float portSpacing = 20.0f;

        // Input Ports (Left)
        for (size_t i = 0; i < m_inputPorts.size(); ++i) {
            m_inputPorts[i]->setBounds(-6, portStartY + i * portSpacing, 12, 12);
        }

        // Output Ports (Right)
        for (size_t i = 0; i < m_outputPorts.size(); ++i) {
            m_outputPorts[i]->setBounds(m_bounds.w - 6, portStartY + i * portSpacing, 12, 12);
        }

        m_deleteBtnBounds = {m_bounds.w - 25, 0, 25, HEADER_H};

        // Editor Content - use editor's PREFERRED size, not stretch to fill
        if (m_editorComponent) {
            float pw = 0, ph = 0;
            m_editorComponent->getPreferredSize(pw, ph);
            
            // Position editor at top-left of content area with its preferred size
            float editorX = PADDING;
            float editorY = HEADER_H + PADDING;
            
            // If the module was auto-sized to be larger than preferred, center it or keep at padding?
            // Let's use the full available width minus padding
            float availableW = m_bounds.w - PADDING * 2;
            m_editorComponent->setBounds(editorX, editorY, availableW, ph);
            m_editorComponent->resized();
        }
    }

    void paint(QuadBatcher& batcher) override;

    void update(float dt) override {
        if (m_node) m_node->updateVisuals();
        Component::update(dt);
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        Component::render(batcher, dt, screenW, screenH);
    }

    std::shared_ptr<FluxNode> getNode() const { return m_node; }

    void setNode(std::shared_ptr<FluxNode> newNode) {
        if (m_node != newNode) {
            m_node = newNode;
            if (m_node) {
                setName(m_node->getName());
                autoGenerateUI();
            }
        }
    }

    std::vector<std::shared_ptr<Port>>& getInputPorts() { return m_inputPorts; }
    std::vector<std::shared_ptr<Port>>& getOutputPorts() { return m_outputPorts; }

    std::shared_ptr<Port> getSidechainPort() {
        for (auto& p : m_inputPorts) {
            if (p->getType() == PortType::Sidechain) return p;
        }
        return nullptr;
    }

    std::function<void(AudioModule*)> onDeleteRequested;

protected:
    std::shared_ptr<FluxNode> m_node;
    std::vector<std::shared_ptr<Port>> m_inputPorts;
    std::vector<std::shared_ptr<Port>> m_outputPorts;
    std::shared_ptr<Component> m_editorComponent;
    Rect m_deleteBtnBounds;
    AudioDeviceManager* m_deviceManager = nullptr;
    void* m_nativeWindowHandle = nullptr;

private:
    size_t m_nodeId;
};

} // namespace Beam

#endif
