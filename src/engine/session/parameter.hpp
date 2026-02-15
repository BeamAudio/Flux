#ifndef PARAMETER_HPP
#define PARAMETER_HPP

#include <string>
#include <atomic>
#include <functional>
#include <algorithm>
#include <mutex>
#include <vector>
#include <iostream>
#include <memory>
#include "engine/session/parameter_queue.hpp"

namespace Beam {

enum class MappingType {
    Linear,
    Logarithmic, // For Frequency
    Skewed       // For general skew
};

class Parameter : public std::enable_shared_from_this<Parameter> {
public:
    Parameter(const std::string& name, float min, float max, float initialValue, MappingType mapping = MappingType::Linear, float skew = 1.0f)
        : m_name(name), m_min(min), m_max(max), m_value(initialValue), m_initialValue(initialValue), m_mapping(mapping), m_skew(skew) 
    {
        m_targetValue.store(initialValue);
        m_currentValue = initialValue;
    }

    float getValue() const {
        return m_value.load(std::memory_order_relaxed);
    }

    float getInitialValue() const { return m_initialValue; }

    /**
     * @brief Prepares a linear ramp for the next processing block.
     * Called by the engine at the start of each audio block.
     */
    void prepareRamp(int frames) {
        m_startValue = m_currentValue;
        m_targetValueInternal = m_targetValue.load(std::memory_order_relaxed);
        m_rampDelta = (m_targetValueInternal - m_startValue) / (float)(frames > 0 ? frames : 1);
        m_rampProgress = 0;
    }

    /**
     * @brief Returns the next sample-accurate value for this parameter using linear interpolation.
     */
    float getNextValue() {
        m_currentValue = m_startValue + m_rampDelta * (float)m_rampProgress;
        m_rampProgress++;
        return m_currentValue;
    }

    void setValue(float newValue) {
        float clamped = std::clamp(newValue, m_min, m_max);
        m_value.store(clamped, std::memory_order_relaxed);
        m_targetValue.store(clamped, std::memory_order_relaxed);
        // If we set value manually (UI), jump immediately or wait for next prepareRamp?
        // UI changes usually benefit from a quick ramp too, so we just update target.
        ParameterQueue::get().push(this, clamped);
    }

    void addListener(std::function<void(float)> listener) {
        std::lock_guard<std::mutex> lock(m_listenerMutex);
        m_listeners.push_back(listener);
    }

    void triggerCallback(float val) {
        std::lock_guard<std::mutex> lock(m_listenerMutex);
        for (auto& listener : m_listeners) {
            if (listener) listener(val);
        }
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

    // Low-level access for FluxGraph optimization
    const std::atomic<float>* getTargetValueAtomic() const { return &m_targetValue; }

private:
    std::string m_name;
    float m_min;
    float m_max;
    float m_initialValue;
    std::atomic<float> m_value; // "Current" value for UI
    std::atomic<float> m_targetValue; // Target for audio thread
    
    // Ramping state (audio thread only, except via prepareRamp)
    float m_currentValue; 
    float m_startValue = 0;
    float m_targetValueInternal = 0;
    float m_rampDelta = 0;
    int m_rampProgress = 0;

    MappingType m_mapping;
    float m_skew;
    
    std::mutex m_listenerMutex;
    std::vector<std::function<void(float)>> m_listeners;
};

} // namespace Beam

#endif // PARAMETER_HPP






