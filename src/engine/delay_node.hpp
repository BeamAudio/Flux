#ifndef DELAY_NODE_HPP
#define DELAY_NODE_HPP

#include "audio_node.hpp"
#include "dsp_utils.hpp"
#include <vector>

namespace Beam {

class DelayNode : public AudioNode {
public:
    DelayNode(float maxDelaySeconds, float feedback, float sampleRate) 
        : m_feedback(feedback), m_sampleRate(sampleRate), m_writePtr(0) {
        size_t maxSamples = static_cast<size_t>(maxDelaySeconds * sampleRate);
        // Ensure buffer is large enough and multiple of channels (2 for stereo)
        m_buffer.resize(maxSamples * 2, 0.0f); 
        setDelayTime(maxDelaySeconds * 0.5f); // Default to half max
    }

    void setDelayTime(float seconds) {
        m_delaySamples = static_cast<size_t>(seconds * m_sampleRate);
        size_t maxDelay = m_buffer.size() / 2;
        if (m_delaySamples >= maxDelay) m_delaySamples = maxDelay - 1;
        if (m_delaySamples < 1) m_delaySamples = 1;
    }

    void setFeedback(float feedback) {
        m_feedback = feedback;
    }

    void process(float* buffer, int frames, int channels, size_t startFrame = 0) override {
        size_t bufferLen = m_buffer.size(); // Total floats
        
        for (int i = 0; i < frames; ++i) {
            for (int c = 0; c < channels; ++c) {
                size_t idx = i * channels + c;
                
                // Calculate read index: (writePtr - delay_in_frames * channels)
                // We need to handle wrapping correctly.
                // Since writePtr is the absolute index in the interleaved buffer:
                // We should track write frame index instead?
                // Let's stick to simple pointer arithmetic.
                
                // Effective delay offset in floats (samples)
                size_t offset = m_delaySamples * channels;
                
                size_t readIdx = m_writePtr + c; // Start at current write pos for channel c
                if (readIdx < offset) {
                    readIdx += bufferLen - offset;
                } else {
                    readIdx -= offset;
                }
                
                float delayedSample = m_buffer[readIdx];
                float inputSample = buffer[idx];
                
                // Output mix: dry + wet
                buffer[idx] = inputSample + delayedSample; 
                
                // Write back to delay line with feedback
                size_t writeIdx = m_writePtr + c;
                m_buffer[writeIdx] = flush_denormal(inputSample + delayedSample * m_feedback);
            }
            
            m_writePtr += channels;
            if (m_writePtr >= bufferLen) m_writePtr = 0;
        }
    }

    std::string getName() const override { return "Delay"; }

private:
    std::vector<float> m_buffer;
    float m_feedback;
    float m_sampleRate;
    size_t m_writePtr;
    size_t m_delaySamples;
};

} // namespace Beam

#endif // DELAY_NODE_HPP





