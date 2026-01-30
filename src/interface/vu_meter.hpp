#ifndef VU_METER_HPP
#define VU_METER_HPP

#include "component.hpp"
#include <cmath>
#include <algorithm>

namespace Beam {

class VUMeter : public Component {
public:
    VUMeter() : m_level(0.0f), m_targetLevel(0.0f) {
        setName("VUMeter");
        setBounds(0, 0, 100, 60);
    }

    void setLevel(float level) { 
        m_targetLevel = level;
    }

    void paint(QuadBatcher& batcher) override;

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        // Analog Ballistics (Physics)
        float attack = 15.0f;
        float release = 3.5f;
        float coeff = (m_targetLevel > m_level) ? attack : release;
        m_level += (m_targetLevel - m_level) * coeff * dt;
        m_level = (std::clamp)(m_level, 0.0f, 1.2f);

        Component::render(batcher, dt, screenW, screenH);
    }

    float getLevel() const { return m_level; }

private:
    float m_level;
    float m_targetLevel;
};

} // namespace Beam

#endif
