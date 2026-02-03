#ifndef METER_SOURCE_HPP
#define METER_SOURCE_HPP

#include <atomic>
#include <vector>
#include <string>
#include <memory>

namespace Beam {

/**
 * @struct MeterData
 * @brief Internal storage for a single meter's state.
 */
struct MeterData {
    std::string name;
    std::atomic<float> value{0.0f};
    std::atomic<float> peak{0.0f};
};

/**
 * @class MeterSource
 * @brief Thread-safe publisher for node-level metering data.
 * Nodes can publish multiple meters (e.g. Input L, Input R, GR).
 */
class MeterSource {
public:
    void addMeter(const std::string& name) {
        auto m = std::make_unique<MeterData>();
        m->name = name;
        m_meters.push_back(std::move(m));
    }

    /**
     * @brief Updates the meter value with basic ballistics. Safe to call from audio thread.
     */
    void updateMeter(size_t index, float value) {
        if (index < m_meters.size()) {
            float current = m_meters[index]->value.load(std::memory_order_relaxed);
            // Instant attack, smooth decay
            float newValue = value;
            if (value < current) {
                newValue = current * 0.95f + value * 0.05f; // Fast decay
            }
            m_meters[index]->value.store(newValue, std::memory_order_relaxed);
            
            // Atomic max for peak
            float currentPeak = m_meters[index]->peak.load(std::memory_order_relaxed);
            while (value > currentPeak && !m_meters[index]->peak.compare_exchange_weak(currentPeak, value)) {
                // Keep trying
            }
        }
    }

    float getValue(size_t index) const {
        return (index < m_meters.size()) ? m_meters[index]->value.load(std::memory_order_relaxed) : 0.0f;
    }

    float getPeak(size_t index) const {
        return (index < m_meters.size()) ? m_meters[index]->peak.load(std::memory_order_relaxed) : 0.0f;
    }

    void resetPeak(size_t index) {
        if (index < m_meters.size()) m_meters[index]->peak.store(0.0f, std::memory_order_relaxed);
    }

    size_t getNumMeters() const { return m_meters.size(); }
    std::string getMeterName(size_t index) const { return (index < m_meters.size()) ? m_meters[index]->name : ""; }

private:
    std::vector<std::unique_ptr<MeterData>> m_meters;
};

} // namespace Beam

#endif // METER_SOURCE_HPP
