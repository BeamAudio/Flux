#ifndef METER_HPP
#define METER_HPP

#include "interface/core/component.hpp"
#include <vector>
#include <cmath>
#include <algorithm>

namespace Beam {

/**
 * @class LuminousMeter
 * @brief A professional LED-style level meter with customizable ballistics.
 */
class LuminousMeter : public Component {
public:
    enum class Orientation { Vertical, Horizontal };

    LuminousMeter(Orientation orient = Orientation::Vertical) 
        : m_orientation(orient), m_level(0.0f), m_peak(0.0f) {
        setName("LevelMeter");
    }

    void setLevel(float linearLevel) {
        if (linearLevel > m_level) m_level = linearLevel; // Instant attack
        if (linearLevel > m_peak) m_peak = linearLevel;
    }

    void paint(QuadBatcher& batcher) override;

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        updateBallistics(dt);
        Component::render(batcher, dt, screenW, screenH);
    }

    void updateBallistics(float dt) {
        m_level -= 1.2f * dt;
        if (m_level < 0.0f) m_level = 0.0f;
        m_peak -= 0.3f * dt;
        if (m_peak < 0.0f) m_peak = 0.0f;
    }

    void setOrientation(Orientation o) { m_orientation = o; }
    Orientation getOrientation() const { return m_orientation; }
    float getLevel() const { return m_level; }
    float getPeak() const { return m_peak; }

private:
    Orientation m_orientation;
    float m_level;
    float m_peak;
};

} // namespace Beam

#endif
