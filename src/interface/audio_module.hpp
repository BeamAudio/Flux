#ifndef AUDIO_MODULE_HPP
#define AUDIO_MODULE_HPP

#include "component.hpp"
#include "port.hpp"
#include "meter.hpp"
#include "slider_modular.hpp"
#include "combo_box.hpp"
#include "../engine/flux_node.hpp"
#include "../engine/input_node.hpp"
#include "../engine/audio_device_manager.hpp"
#include "../utilities/flux_audio_utils.hpp"
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
            // addChildComponent(m_inputPort); // Handled by Workspace to avoid clipping/hit-test issues
        }
            
        if (node->getOutputPorts().size() > 0) {
            m_outputPort = std::make_shared<Port>(PortType::Output, this);
            // addChildComponent(m_outputPort); // Handled by Workspace
        }
        
        setDraggable(true);
        setClipsChildren(false); // Allow ports to overhang
        autoGenerateUI();
        updateLayout();
        
        setBounds(x, y, m_bounds.w, m_bounds.h);
    }

    size_t getNodeId() const { return m_nodeId; }

    /**
     * @brief Populates the module with controls based on the node's parameters.
     */
    virtual void autoGenerateUI() {
        if (!m_node) return;
        m_children.clear();

        // 1. Special case for Audio Input (Meter + ComboBox)
        if (getName() == "Audio Input") {
            auto meter = std::make_shared<LuminousMeter>(LuminousMeter::Orientation::Horizontal);
            meter->setName("LevelMeter");
            addChildComponent(meter);

            if (m_deviceManager) {
                auto combo = std::make_shared<ComboBox>();
                combo->setName("InputSelector");
                auto devices = m_deviceManager->getAvailableInputDevices();
                auto current = m_deviceManager->getCurrentDeviceSetup().inputDeviceId;
                int idx = 0;
                for (int i=0; i<(int)devices.size(); ++i) {
                    combo->addItem(devices[i].name);
                    if (devices[i].deviceId == current) idx = i;
                }
                combo->setSelectedId(idx);
                combo->setOnChange([this, devices](int index) {
                    if (index >= 0 && index < (int)devices.size()) {
                        auto setup = m_deviceManager->getCurrentDeviceSetup();
                        m_deviceManager->setCurrentAudioDevice(
                            setup.outputDeviceName, setup.outputDeviceId,
                            devices[index].name, devices[index].deviceId,
                            setup.sampleRate, setup.blockSize);
                    }
                });
                addChildComponent(combo);
            }
        }

        // 2. Add Sliders for all parameters
        for (auto const& [name, param] : m_node->getParameters()) {
            if (name == "Master Gain" || name == "Capture Mode") continue;
            
            auto slider = std::make_shared<ModularSlider>(name, ModularSlider::Style::Horizontal);
            slider->bindParameter(param);
            addChildComponent(slider);
        }
    }

    /**
     * @brief Calculates dimensions and positions of internal elements.
     */
    void updateLayout() {
        float curY = 40.0f; // Header padding
        float width = 160.0f;
        
        // Ensure width is sufficient for name
        float tw = AudioUtils::calculateTextWidth(getName(), 12.0f);
        width = (std::max)(width, tw + 60.0f);

        for (auto& child : m_children) {
            if (child.get() == m_inputPort.get() || child.get() == m_outputPort.get()) continue;
            float h = 30.0f;
            if (child->getName() == "LevelMeter") h = 15.0f;
            if (child->getName() == "InputSelector") h = 24.0f;
            
            child->setBounds(m_bounds.x + 10, m_bounds.y + curY, width - 20, h);
            curY += h + 10.0f;
        }

        m_bounds.w = width;
        m_bounds.h = (std::max)(100.0f, curY + 10.0f);
    }

    void setBounds(float x, float y, float w, float h) override {
        Component::setBounds(x, y, w, h);
        
        if (m_inputPort) m_inputPort->setBounds(x - 6, y + 50, 12, 12);
        if (m_outputPort) m_outputPort->setBounds(x + w - 6, y + 50, 12, 12);
        
        m_deleteBtnBounds = {x + w - 25, y, 25, 30};
        
        // Position children relative to the new top-left
        float curY = 40.0f;
        for(auto& child : m_children) {
            if (child.get() == m_inputPort.get() || child.get() == m_outputPort.get()) continue;
            Rect b = child->getBounds();
            child->setBounds(x + 10, y + curY, w - 20, b.h);
            curY += b.h + 10.0f;
        }
    }

    void paint(QuadBatcher& batcher) override;

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        // Update Dynamic Components (like Meters)
        if (getName() == "Audio Input") {
            auto inputNode = std::dynamic_pointer_cast<InputNode>(m_node);
            for(auto& child : m_children) {
                if (child->getName() == "LevelMeter") {
                    auto meter = std::dynamic_pointer_cast<LuminousMeter>(child);
                    if (meter && inputNode) meter->setLevel(inputNode->getPeakLevel());
                }
            }
        }

        Component::render(batcher, dt, screenW, screenH);
    }

    bool onMouseDown(float x, float y, int button) override {
        if (getName() != "Master" && getName() != "MASTER" && m_deleteBtnBounds.contains(x, y)) {
            if (onDeleteRequested) onDeleteRequested(this);
            return true;
        }
        
        return Component::onMouseDown(x, y, button);
    }

    std::shared_ptr<Port> getInputPort() { return m_inputPort; }
    std::shared_ptr<Port> getOutputPort() { return m_outputPort; }

    std::function<void(AudioModule*)> onDeleteRequested;

protected:
    std::shared_ptr<FluxNode> m_node;
    std::shared_ptr<Port> m_inputPort;
    std::shared_ptr<Port> m_outputPort;
    Rect m_deleteBtnBounds;
    AudioDeviceManager* m_deviceManager = nullptr;

private:
    size_t m_nodeId;
};

} // namespace Beam

#endif
