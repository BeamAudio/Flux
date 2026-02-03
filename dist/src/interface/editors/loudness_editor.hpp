#ifndef LOUDNESS_EDITOR_HPP
#define LOUDNESS_EDITOR_HPP

#include "interface/core/component.hpp"
#include "interface/core/theme.hpp"
#include "engine/plugins/flux_plugin.hpp"
#include <string>
#include <algorithm>
#include "interface/widgets/crt_loudness_meter.hpp"

namespace Beam {

class LoudnessEditor : public Component {
public:
    LoudnessEditor(FluxPlugin* node) : m_node(node) {
        setName("LoudnessEditor");
        m_meter = std::make_shared<CRTLoudnessMeter>();
        addChildComponent(m_meter);
    }

    void getPreferredSize(float& w, float& h) const override {
        w = 240;
        h = 160;
    }
    
    void setBounds(float x, float y, float width, float height) override {
        Component::setBounds(x, y, width, height);
        if (m_meter) m_meter->setBounds(0, 0, width, height);
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        if (m_node && m_meter) {
            m_node->updateVisuals(); // Update plugin params from processor
            
            auto pM = m_node->getParameter("Momentary");
            auto pS = m_node->getParameter("ShortTerm");
            auto pT = m_node->getParameter("True Peak");
            
            float m = pM ? pM->getValue() : -60.0f;
            float s = pS ? pS->getValue() : -60.0f;
            float t = pT ? pT->getValue() : -60.0f;
            
            m_meter->setUpdate(m, s, t);
        }
        Component::render(batcher, dt, screenW, screenH);
    }

private:
    FluxPlugin* m_node;
    std::shared_ptr<CRTLoudnessMeter> m_meter;
};

} // namespace Beam
#endif
