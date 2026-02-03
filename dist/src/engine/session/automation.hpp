#ifndef AUTOMATION_HPP
#define AUTOMATION_HPP

#include <vector>
#include <algorithm>
#include <memory>
#include "engine/session/parameter.hpp"

namespace Beam {

/**
 * @struct AutomationPoint
 * @brief A single point in an automation lane.
 */
struct AutomationPoint {
    size_t frame; ///< Timeline position in frames
    float value;  ///< Parameter value at this position
    float curvature = 0.0f; ///< 0 = Linear, -1 to 1 = Bezier curve intensity
};

/**
 * @class AutomationLane
 * @brief Manages a sequence of automation points for a single parameter.
 */
class AutomationLane {
public:
    explicit AutomationLane(std::shared_ptr<Parameter> param) : m_parameter(param) {
        if (m_parameter) {
            m_parameter->addListener([this](float val) {
                // Debug listener
                // std::cout << "AutoLane Listener: " << val << " Points: " << m_points.size() << std::endl;
                
                if (m_recording) {
                     if (m_points.size() == 1 && m_points[0].frame == 0) {
                         m_points[0].value = val;
                     }
                } else {
                     if (m_points.size() == 1 && m_points[0].frame == 0) {
                         m_points[0].value = val;
                     } else if (m_points.empty()) {
                         // If empty, maybe add one?
                         // m_points.push_back({0, val});
                     }
                }
            });
        }
    }

    /**
     * @brief Adds or updates a point at a specific frame.
     */
    void addPoint(size_t frame, float value) {
        // Basic thinning: if the last point has the same value, update its end time if possible, 
        // but for now let's just add it and sort. 
        // OPTIMIZATION: Check if we can just update the last point if it's very close in time or value.
        
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

    void recordPoint(size_t frame, float value) {
        if (!m_recording) return;
        
        // Thinning: Don't record if value hasn't changed significantly from last point
        if (!m_points.empty()) {
            const auto& last = m_points.back();
            if (std::abs(last.value - value) < 0.0001f && frame > last.frame) {
                // Determine if we need an intermediate point? 
                // For now, simple "recording changes" logic. 
                // If value is stable, we might want to just update a "hold" or just skip.
                // Let's skip for now to save data.
                return;
            }
        }
        addPoint(frame, value);
    }

    void setRecording(bool rec) { m_recording = rec; }
    bool isRecording() const { return m_recording; }

    void setOverride(bool ovr) { m_override = ovr; }
    bool isOverride() const { return m_override; }

    void clear() { m_points.clear(); }
    
    /**
     * @brief Updates an existing point at the given index.
     */
    void updatePoint(size_t index, size_t newFrame, float newValue) {
        if (index >= m_points.size()) return;
        m_points[index].frame = newFrame;
        m_points[index].value = newValue;
        // Re-sort to maintain order
        std::sort(m_points.begin(), m_points.end(), [](const AutomationPoint& a, const AutomationPoint& b) {
            return a.frame < b.frame;
        });
    }

    void setCurvature(size_t index, float curvature) {
        if (index >= m_points.size()) return;
        m_points[index].curvature = std::clamp(curvature, -1.0f, 1.0f);
    }

    /**
     * @brief Interpolates the value for a specific frame.
     */
    float getValueAt(size_t frame) const {
        if (m_points.empty()) return m_parameter ? m_parameter->getValue() : 0.0f;
        if (frame <= m_points.front().frame) return m_points.front().value;
        if (frame >= m_points.back().frame) return m_points.back().value;

        // Find segments - Linear Search is slow, optimize to binary search later
        auto it = std::lower_bound(m_points.begin(), m_points.end(), frame, [](const AutomationPoint& p, size_t f) {
            return p.frame < f;
        });

        if (it == m_points.begin()) return it->value;
        
        const auto& p2 = *it;
        const auto& p1 = *(it - 1);
        
        if (p2.frame == p1.frame) return p1.value;

        float t = (float)(frame - p1.frame) / (float)(p2.frame - p1.frame);
        
        // Curved interpolation (Power-based Bezier approximation)
        if (p1.curvature != 0.0f) {
            if (p1.curvature > 0) {
                // Convex
                t = std::pow(t, 1.0f + p1.curvature * 4.0f);
            } else {
                // Concave
                t = 1.0f - std::pow(1.0f - t, 1.0f - p1.curvature * 4.0f);
            }
        }
        
        return p1.value + t * (p2.value - p1.value);
    }

    /**
     * @brief Applies the interpolated value to the linked parameter.
     * Does nothing if override is enabled (manual control) or recording.
     */
    void applyAt(size_t frame) {
        if (m_parameter && !m_recording && !m_override) {
            m_parameter->setValue(getValueAt(frame));
        }
    }

    std::shared_ptr<Parameter> getParameter() { return m_parameter; }
    
    std::vector<AutomationPoint>& getPoints() { return m_points; }

private:
    std::shared_ptr<Parameter> m_parameter;
    std::vector<AutomationPoint> m_points;
    bool m_recording = false;
    bool m_override = false;
};

} // namespace Beam

#endif // AUTOMATION_HPP






