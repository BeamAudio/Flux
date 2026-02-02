#include "engine/nodes/input_node.hpp"
#include "engine/core/audio_device_manager.hpp"
#include "interface/widgets/meter.hpp"
#include "interface/widgets/combo_box.hpp"
#include "interface/core/layout.hpp"
#include "interface/analog/analog_ui_templates.hpp"

namespace Beam {

/**
 * @class InputNodeEditor
 * @brief Custom editor for Hardware Input. Uses Rack styling.
 */
class InputNodeEditor : public Component {
public:
    InputNodeEditor(InputNode* node, AudioDeviceManager* dm) {
        // 1. Rack UI (Background and Title)
        auto style = RackUnitUI::Utility("HARDWARE INPUT");
        style.chassisColor = {0.75f, 0.75f, 0.78f, 1.0f}; // Brushed Aluminum
        style.textColor = {0.1f, 0.1f, 0.1f, 1.0f};
        style.showMeter = true; 
        style.invertMeter = true; // Input grows bottom-up
        
        m_rackUI = std::make_shared<RackUnitUI>(node, style);
        addChildComponent(m_rackUI);

        // 2. Device Selection Combo (sitting on the rack face)
        if (dm) {
            m_combo = std::make_shared<ComboBox>();
            auto devices = dm->getAvailableInputDevices();
            auto current = node->getDeviceId();
            int idx = 0;
            for (int i=0; i<(int)devices.size(); ++i) {
                m_combo->addItem(devices[i].name);
                if (devices[i].deviceId == current) idx = i;
            }
            m_combo->setSelectedId(idx);
            
            // Register current device (including default "")
            dm->openInputStream(current);

            m_combo->setOnChange([dm, devices, node](int index) {
                if (index >= 0 && index < (int)devices.size()) {
                    auto deviceId = devices[index].deviceId;
                    node->setDeviceId(deviceId);
                    dm->openInputStream(deviceId);
                    std::cout << "InputNode switched to device: " << devices[index].name << std::endl;
                }
            });
            addChildComponent(m_combo);
        }
        
        m_node = node;
    }

    void getPreferredSize(float& w, float& h) const override {
        float rackW = 0, rackH = 0;
        m_rackUI->getPreferredSize(rackW, rackH);
        w = (std::max)(rackW, 220.0f);
        h = (std::max)(rackH, 60.0f) + 30.0f;
    }

    void resized() override {
        float rackW = 0, rackH = 0;
        m_rackUI->getPreferredSize(rackW, rackH);
        m_rackUI->setBounds(0, 0, m_bounds.w, rackH);
        
        if (m_combo) {
            m_combo->setBounds(20, rackH - 5, m_bounds.w - 60, 24);
        }
    }

    void render(QuadBatcher& batcher, float dt, float w, float h) override {
        // Feed node peak to meter source for RackUnitUI to pick up
        if (m_node) {
            m_node->getMeterSource()->updateMeter(0, m_node->getPeakLevel());
        }
        Component::render(batcher, dt, w, h);
    }

private:
    std::shared_ptr<RackUnitUI> m_rackUI;
    std::shared_ptr<ComboBox> m_combo;
    InputNode* m_node;
};

// Implement createEditor
std::shared_ptr<Component> InputNode::createEditor(const NodeEditorContext& context) {
    return std::make_shared<InputNodeEditor>(this, context.deviceManager);
}

} // namespace Beam