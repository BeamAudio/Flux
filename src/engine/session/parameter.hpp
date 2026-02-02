#ifndef PARAMETER_HPP
#define PARAMETER_HPP

#include <string>
#include <atomic>
#include <functional>
#include <algorithm>
#include "engine/session/parameter_queue.hpp"

namespace Beam {

enum class MappingType {
    Linear,
    Logarithmic, // For Frequency
    Skewed       // For general skew
};

class Parameter {
public:
    Parameter(const std::string& name, float min, float max, float initialValue, MappingType mapping = MappingType::Linear, float skew = 1.0f)
        : m_name(name), m_min(min), m_max(max), m_value(initialValue), m_mapping(mapping), m_skew(skew) 
    {
        m_targetValue.store(initialValue);
        m_currentValue = initialValue;
    }

    float getValue() const {
        return m_value.load(std::memory_order_relaxed);
    }

    /**
     * @brief Returns the next sample-accurate value for this parameter.
     * Call this inside the sample loop of your process method.
     */
    float getNextValue() {
        float target = m_targetValue.load(std::memory_order_relaxed);
        // Simple one-pole smoothing
        m_currentValue = m_currentValue * 0.999f + target * 0.001f;
        return m_currentValue;
    }

    void setValue(float newValue) {
        float clamped = std::clamp(newValue, m_min, m_max);
        m_value.store(clamped, std::memory_order_relaxed);
        m_targetValue.store(clamped, std::memory_order_relaxed);
        ParameterQueue::get().push(this, clamped);
    }

    void triggerCallback(float val) {
        if (onChanged) onChanged(val);
    }

    float getNormalizedValue() const {
        float val = getValue();
        if (m_mapping == MappingType::Logarithmic && m_min > 0) {
            return std::log(val / m_min) / std::log(m_max / m_min);
        }
        if (m_skew != 1.0f) {
            float norm = (val - m_min) / (m_max - m_min);
            return std::pow(norm, 1.0f / m_skew);
        }
        return (val - m_min) / (m_max - m_min);
    }

    void setNormalizedValue(float norm) {
        float val;
        if (m_mapping == MappingType::Logarithmic && m_min > 0) {
            val = m_min * std::exp(norm * std::log(m_max / m_min));
        } else if (m_skew != 1.0f) {
            val = m_min + std::pow(norm, m_skew) * (m_max - m_min);
        } else {
            val = m_min + norm * (m_max - m_min);
        }
        setValue(val);
    }

    const std::string& getName() const { return m_name; }
    float getMin() const { return m_min; }
    float getMax() const { return m_max; }

    std::function<void(float)> onChanged;
    
    // Low-level access for FluxGraph optimization
    const std::atomic<float>* getTargetValueAtomic() const { return &m_targetValue; }

private:
    std::string m_name;
    float m_min;
    float m_max;
    std::atomic<float> m_value; // "Current" value for UI
    std::atomic<float> m_targetValue; // Target for audio thread
    float m_currentValue; // Audio thread local smoothed value
    MappingType m_mapping;
    float m_skew;
};

} // namespace Beam

#endif // PARAMETER_HPP






