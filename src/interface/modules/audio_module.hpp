#ifndef AUDIO_MODULE_HPP
#define AUDIO_MODULE_HPP

#include "interface/core/component.hpp"
#include "interface/modules/port.hpp"
#include "interface/widgets/button.hpp"
#include "interface/core/layout.hpp"
#include "interface/editors/generic_node_editor.hpp"
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
    AudioModule(std::shared_ptr<FluxNode> node, size_t nodeId, float x, float y, AudioDeviceManager* deviceManager = nullptr) 
        : m_node(node), m_nodeId(nodeId), m_deviceManager(deviceManager) {
        
        setName(node->getName());

        if (node->getInputPorts().size() > 0) {
            m_inputPort = std::make_shared<Port>(PortType::Input, this);
            addChildComponent(m_inputPort); 
        }
            
        if (node->getOutputPorts().size() > 0 && node->getName() != "Master") {
            m_outputPort = std::make_shared<Port>(PortType::Output, this);
            addChildComponent(m_outputPort); 
        }

        for (const auto& p : node->getInputPorts()) {
            if (p.type == FluxNode::Port::Sidechain) {
                m_sidechainPort = std::make_shared<Port>(PortType::Sidechain, this);
                addChildComponent(m_sidechainPort);
                break;
            }
        }
        
        setDraggable(true);
        setClipsChildren(false); // Allow ports to overhang
        
        setBounds(x, y, 160, 100); // Initial size
        autoGenerateUI(); 
    }

    size_t getNodeId() const { return m_nodeId; }

    virtual void autoGenerateUI() {
        if (!m_node) return;
        m_children.clear();
        
        // Restore ports
        if (m_inputPort) addChildComponent(m_inputPort);
        if (m_outputPort) addChildComponent(m_outputPort);
        if (m_sidechainPort) addChildComponent(m_sidechainPort);

        // Create Context
        NodeEditorContext ctx;
        ctx.deviceManager = m_deviceManager;

        m_editorComponent = m_node->createEditor(ctx);

        if (!m_editorComponent) {
             m_editorComponent = std::make_shared<GenericNodeEditor>(m_node.get());
        }

        if (m_editorComponent) {
            // Adaptive: Allow editor to grow
            m_editorComponent->setClipsChildren(false); 
            addChildComponent(m_editorComponent);
            
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
        
        if (m_sidechainPort) totalH += 15.0f;

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
        auto check = [&](std::shared_ptr<Port> p) {
            if (p && p->getBounds().contains(localX, localY)) {
                // Pass coords relative to PORT
                return p->onMouseDown(localX - p->getX(), localY - p->getY(), button, shift);
            }
            return false;
        };
        
        if (check(m_inputPort)) return true;
        if (check(m_outputPort)) return true;
        if (check(m_sidechainPort)) return true;
        
        if (getName() != "Master" && getName() != "MASTER" && m_deleteBtnBounds.contains(localX, localY)) {
            if (onDeleteRequested) onDeleteRequested(this);
            return true;
        }
        
        // Pass original Parent Coords to base, it handles conversion
        return Component::onMouseDown(x, y, button, shift);
    }

    void resized() override {
        // Layout using local coordinates with consistent constants
        static constexpr float HEADER_H = 30.0f;
        static constexpr float PADDING = 8.0f;
        float portOffset = 50.0f;

        // Ports (Local)
        if (m_inputPort) m_inputPort->setBounds(-6, portOffset, 12, 12); 
        if (m_outputPort) m_outputPort->setBounds(m_bounds.w - 6, portOffset, 12, 12);
        
        if (m_sidechainPort) {
            m_sidechainPort->setBounds((m_bounds.w - 12) * 0.5f, m_bounds.h - 6, 12, 12);
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

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        Component::render(batcher, dt, screenW, screenH);
    }

    std::shared_ptr<Port> getInputPort() { return m_inputPort; }
    std::shared_ptr<Port> getOutputPort() { return m_outputPort; }
    std::shared_ptr<Port> getSidechainPort() { return m_sidechainPort; }

    std::function<void(AudioModule*)> onDeleteRequested;

protected:
    std::shared_ptr<FluxNode> m_node;
    std::shared_ptr<Port> m_inputPort;
    std::shared_ptr<Port> m_outputPort;
    std::shared_ptr<Port> m_sidechainPort;
    std::shared_ptr<Component> m_editorComponent;
    Rect m_deleteBtnBounds;
    AudioDeviceManager* m_deviceManager = nullptr;

private:
    size_t m_nodeId;
};

} // namespace Beam

#endif
