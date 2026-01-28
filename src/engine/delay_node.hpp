#ifndef DELAY_NODE_HPP
#define DELAY_NODE_HPP

#include "audio_node.hpp"
#include "dsp_utils.hpp"
#include <vector>

namespace Beam {

class DelayNode : public AudioNode {
public:
    DelayNode(float delaySeconds, float feedback, float sampleRate) 
        : m_feedback(feedback), m_sampleRate(sampleRate), m_writePtr(0) {
        size_t delaySamples = static_cast<size_t>(delaySeconds * sampleRate);
        m_buffer.resize(delaySamples * 2, 0.0f); // Stereo buffer
    }

    void process(float* buffer, int frames, int channels, size_t startFrame = 0) override {
        for (int i = 0; i < frames; ++i) {
            for (int c = 0; c < channels; ++c) {
                size_t idx = i * channels + c;
                float delayedSample = m_buffer[m_writePtr];
                
                float inputSample = buffer[idx];
                buffer[idx] += delayedSample;
                
                m_buffer[m_writePtr] = flush_denormal(inputSample + delayedSample * m_feedback);
                
                m_writePtr++;
                if (m_writePtr >= m_buffer.size()) m_writePtr = 0;
            }
        }
    }

    std::string getName() const override { return "Delay"; }

private:
    std::vector<float> m_buffer;
    float m_feedback;
    float m_sampleRate;
    size_t m_writePtr;
};

} // namespace Beam

#endif // DELAY_NODE_HPP
