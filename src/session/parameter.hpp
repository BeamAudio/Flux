#ifndef PARAMETER_HPP
#define PARAMETER_HPP

#include <string>
#include <atomic>
#include <functional>
#include <algorithm>

namespace Beam {

class Parameter {
public:
    Parameter(const std::string& name, float min, float max, float initialValue)
        : m_name(name), m_min(min), m_max(max), m_value(initialValue) {}

    float getValue() const {
        return m_value.load(std::memory_order_relaxed);
    }

    void setValue(float newValue) {
        float clamped = std::clamp(newValue, m_min, m_max);
        m_value.store(clamped, std::memory_order_relaxed);
        if (onChanged) {
            onChanged(clamped);
        }
    }

    float getNormalizedValue() const {
        return (getValue() - m_min) / (m_max - m_min);
    }

    void setNormalizedValue(float norm) {
        setValue(m_min + norm * (m_max - m_min));
    }

    const std::string& getName() const { return m_name; }
    float getMin() const { return m_min; }
    float getMax() const { return m_max; }

    std::function<void(float)> onChanged;

private:
    std::string m_name;
    float m_min;
    float m_max;
    std::atomic<float> m_value;
};

} // namespace Beam

#endif // PARAMETER_HPP



