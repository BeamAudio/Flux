#ifndef NEEDLE_METER_HPP
#define NEEDLE_METER_HPP

#include "interface/widgets/vu_meter.hpp"
#include "interface/core/look_and_feel.hpp"
#include <cmath>
#include <algorithm>

namespace Beam {

/**
 * @class NeedleMeter
 * @brief An analog-style VU meter with physics-based needle movement.
 */
class NeedleMeter : public VUMeter {
public:
    NeedleMeter() : m_value(0.0f), m_targetValue(0.0f), m_velocity(0.0f) {
        setName("NeedleMeter");
    }

    void setValue(float val) { m_targetValue = std::clamp(val, 0.0f, 1.2f); }
    float getValue() const { return m_value; }

    void getPreferredSize(float& w, float& h) const override {
        w = 100.0f; h = 65.0f;
    }

    void paint(QuadBatcher& batcher) override {
        // Drawing is handled by LookAndFeel
        getLookAndFeel().drawVUMeter(batcher, *this, m_value);
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        // Physics-based ballistics (Mass-Spring-Damper model for analog feel)
        // Adjust these for the "weight" of the needle
        const float stiffness = 120.0f;
        const float damping = 12.0f;

        float force = (m_targetValue - m_value) * stiffness;
        float drag = m_velocity * damping;
        float acceleration = force - drag;

        m_velocity += acceleration * dt;
        m_value += m_velocity * dt;

        // Clip and bounce slightly
        if (m_value < 0.0f) { m_value = 0.0f; m_velocity *= -0.2f; }
        if (m_value > 1.2f) { m_value = 1.2f; m_velocity *= -0.2f; }

        Component::render(batcher, dt, screenW, screenH);
    }

private:
    float m_value;
    float m_targetValue;
    float m_velocity;
};

} // namespace Beam

#endif // NEEDLE_METER_HPP
