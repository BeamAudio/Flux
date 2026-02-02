#ifndef PDC_DELAY_NODE_HPP
#define PDC_DELAY_NODE_HPP

#include "engine/core/flux_node.hpp"
#include <vector>
#include <algorithm>

namespace Beam {

/**
 * @class PDCDelayNode
 * @brief High-precision sample delay used for Plugin Delay Compensation.
 * No feedback, no dry/wet mix - just a pure time offset.
 */
class PDCDelayProcessor : public FluxProcessor {
public:
    PDCDelayProcessor(size_t delayFrames, int maxBlockSize) : m_delayFrames(delayFrames) {
        m_ringBuffer.assign((delayFrames + maxBlockSize + 100) * 2, 0.0f);
    }

    void process(const float** inputs, float** outputs, int frames) override {
        const float* in = inputs[0];
        float* out = outputs[0];
        
        if (m_delayFrames == 0) {
            std::copy(in, in + frames * 2, out);
            return;
        }

        size_t cap = m_ringBuffer.size();
        for (int i = 0; i < frames; ++i) {
            for (int c = 0; c < 2; ++c) {
                float sample = in[i * 2 + c];
                m_ringBuffer[m_writePos] = sample;
                
                size_t readPos = (m_writePos + cap - m_delayFrames * 2) % cap;
                out[i * 2 + c] = m_ringBuffer[readPos];
                
                m_writePos = (m_writePos + 1) % cap;
            }
        }
    }

private:
    size_t m_delayFrames;
    std::vector<float> m_ringBuffer;
    size_t m_writePos = 0;
};

class PDCDelayNode : public FluxNode {
public:
    PDCDelayNode(size_t delayFrames, int bufferSize) 
        : m_delayFrames(delayFrames), m_bufferSize(bufferSize) {}

    std::unique_ptr<FluxProcessor> createProcessor() override {
        return std::make_unique<PDCDelayProcessor>(m_delayFrames, m_bufferSize);
    }

    std::string getName() const override { return "PDC Delay"; }
    std::vector<Port> getInputPorts() const override { return { {"In", 2} }; }
    std::vector<Port> getOutputPorts() const override { return { {"Out", 2} }; }

private:
    size_t m_delayFrames;
    int m_bufferSize;
};

} // namespace Beam

#endif
