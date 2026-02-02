#include "engine/nodes/master_node.hpp"
#include "engine/core/audio_device_manager.hpp"
#include "interface/widgets/meter.hpp"
#include "interface/widgets/combo_box.hpp"
#include "interface/core/layout.hpp"
#include "interface/analog/analog_ui_templates.hpp"
#include <iostream>

namespace Beam {

class MasterNodeEditor : public Component {
public:
    MasterNodeEditor(MasterNode* node, AudioDeviceManager* dm) {
        auto style = RackUnitUI::Utility("MASTER OUTPUT");
        style.chassisColor = {0.25f, 0.25f, 0.28f, 1.0f}; // Dark Charcoal
        style.textColor = {0.9f, 0.9f, 0.95f, 1.0f};
        style.showMeter = true;
        
        m_rackUI = std::make_shared<RackUnitUI>(node, style);
        addChildComponent(m_rackUI);

        if (dm) {
            m_combo = std::make_shared<ComboBox>();
            auto devices = dm->getAvailableOutputDevices();
            auto current = dm->getCurrentDeviceSetup().outputDeviceId;
            int idx = 0;
            for (int i=0; i<(int)devices.size(); ++i) {
                m_combo->addItem(devices[i].name);
                if (devices[i].deviceId == current) idx = i;
            }
            m_combo->setSelectedId(idx);
            m_combo->setOnChange([dm, devices](int index) {
                if (index >= 0 && index < (int)devices.size()) {
                    auto setup = dm->getCurrentDeviceSetup();
                    std::cout << "[MasterNodeEditor] Changing to: " << devices[index].name << " ID: " << devices[index].deviceId << std::endl;
                    dm->setCurrentAudioDevice(
                        devices[index].name, devices[index].deviceId,
                        setup.inputDeviceName, setup.inputDeviceId,
                        setup.sampleRate, setup.blockSize);
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
            m_combo->setBounds(20, rackH - 5, m_bounds.w - 40, 24);
        }
    }

    void render(QuadBatcher& batcher, float dt, float w, float h) override {
        if (m_node) {
            m_node->getMeterSource()->updateMeter(0, m_node->getMeterSource()->getValue(0));
            m_node->getMeterSource()->updateMeter(1, m_node->getMeterSource()->getValue(1));
        }
        Component::render(batcher, dt, w, h);
    }

private:
    std::shared_ptr<RackUnitUI> m_rackUI;
    std::shared_ptr<ComboBox> m_combo;
    MasterNode* m_node;
};

std::shared_ptr<Component> MasterNode::createEditor(const NodeEditorContext& context) {
    return std::make_shared<MasterNodeEditor>(this, context.deviceManager);
}

} // namespace Beam
