#ifndef MIXER_STATE_HPP
#define MIXER_STATE_HPP

#include <map>
#include <atomic>
#include <memory>

namespace Beam {

/**
 * @struct MixChannel
 * @brief State for a single mixer channel (node feeding Master).
 * Uses atomics for real-time safe access from audio thread.
 */
struct MixChannel {
    std::atomic<float> gain{1.0f};
    std::atomic<float> pan{0.5f}; // 0.0 = Left, 1.0 = Right
    std::atomic<bool> muted{false};
    std::atomic<bool> solo{false};
    
    // For metering (written by audio thread, read by UI)
    std::atomic<float> peakL{0.0f};
    std::atomic<float> peakR{0.0f};
};

/**
 * @class MixerState
 * @brief Central storage for all mixer channel states.
 * Indexed by source node ID for efficient lookup during RenderPlan compilation.
 */
class MixerState {
public:
    /**
     * @brief Gets or creates a mix channel for the given node ID.
     */
    MixChannel* getOrCreateChannel(size_t nodeId) {
        auto it = m_channels.find(nodeId);
        if (it != m_channels.end()) {
            return it->second.get();
        }
        auto channel = std::make_unique<MixChannel>();
        MixChannel* ptr = channel.get();
        m_channels[nodeId] = std::move(channel);
        return ptr;
    }
    
    MixChannel* getChannel(size_t nodeId) {
        auto it = m_channels.find(nodeId);
        return (it != m_channels.end()) ? it->second.get() : nullptr;
    }
    
    void removeChannel(size_t nodeId) {
        m_channels.erase(nodeId);
    }
    
    const std::map<size_t, std::unique_ptr<MixChannel>>& getChannels() const { return m_channels; }

private:
    std::map<size_t, std::unique_ptr<MixChannel>> m_channels;
};

} // namespace Beam

#endif // MIXER_STATE_HPP
