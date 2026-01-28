#ifndef AUTOMATION_HPP
#define AUTOMATION_HPP

#include <vector>
#include <algorithm>
#include <memory>
#include "parameter.hpp"

namespace Beam {

/**
 * @struct AutomationPoint
 * @brief A single point in an automation lane.
 */
struct AutomationPoint {
    size_t frame; ///< Timeline position in frames
    float value;  ///< Parameter value at this position
};

/**
 * @class AutomationLane
 * @brief Manages a sequence of automation points for a single parameter.
 */
class AutomationLane {
public:
    explicit AutomationLane(std::shared_ptr<Parameter> param) : m_parameter(param) {}

    /**
     * @brief Adds or updates a point at a specific frame.
     */
    void addPoint(size_t frame, float value) {
        auto it = std::find_if(m_points.begin(), m_points.end(), [frame](const AutomationPoint& p) {
            return p.frame == frame;
        });

        if (it != m_points.end()) {
            it->value = value;
        } else {
            m_points.push_back({frame, value});
            std::sort(m_points.begin(), m_points.end(), [](const AutomationPoint& a, const AutomationPoint& b) {
                return a.frame < b.frame;
            });
        }
    }

    /**
     * @brief Interpolates the value for a specific frame.
     */
    float getValueAt(size_t frame) const {
        if (m_points.empty()) return m_parameter ? m_parameter->getValue() : 0.0f;
        if (frame <= m_points.front().frame) return m_points.front().value;
        if (frame >= m_points.back().frame) return m_points.back().value;

        // Find segments
        for (size_t i = 0; i < m_points.size() - 1; ++i) {
            if (frame >= m_points[i].frame && frame <= m_points[i+1].frame) {
                float t = (float)(frame - m_points[i].frame) / (float)(m_points[i+1].frame - m_points[i].frame);
                return m_points[i].value + t * (m_points[i+1].value - m_points[i].value);
            }
        }
        return m_points.back().value;
    }

    /**
     * @brief Applies the interpolated value to the linked parameter.
     */
    void applyAt(size_t frame) {
        if (m_parameter) {
            m_parameter->setValue(getValueAt(frame));
        }
    }

    std::shared_ptr<Parameter> getParameter() { return m_parameter; }

private:
    std::shared_ptr<Parameter> m_parameter;
    std::vector<AutomationPoint> m_points;
};

} // namespace Beam

#endif // AUTOMATION_HPP

