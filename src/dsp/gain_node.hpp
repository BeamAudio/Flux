#ifndef GAIN_NODE_HPP
#define GAIN_NODE_HPP

#include "audio_node.hpp"
#include <atomic>

namespace Beam {

class GainNode : public AudioNode {
public:
    GainNode(float gain = 1.0f) : m_gain(gain) {}

    void process(float* buffer, int frames, int channels) override {
        float currentGain = m_gain.load();
        for (int i = 0; i < frames * channels; ++i) {
            buffer[i] *= currentGain;
        }
    }

    void setGain(float gain) { m_gain.store(gain); }
    std::string getName() const override { return "Gain"; }

private:
    std::atomic<float> m_gain;
};

} // namespace Beam

#endif // GAIN_NODE_HPP
