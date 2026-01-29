#ifndef INPUT_NODE_HPP
#define INPUT_NODE_HPP

#include "flux_node.hpp"
#include <mutex>
#include <vector>

namespace Beam {

/**
 * @class InputNode
 * @brief Provides real-time audio input from the hardware to the Flux Graph.
 */
class InputNode : public FluxNode {
public:
    InputNode(int bufferSize) {
        setupBuffers(0, 1, bufferSize, 2); // 0 inputs, 1 output (stereo)
    }

    void process(int frames) override {
        float* out = getOutputBuffer(0);
        
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        if (m_capturedData.size() >= (size_t)(frames * 2)) {
            std::copy(m_capturedData.begin(), m_capturedData.begin() + frames * 2, out);
            m_capturedData.erase(m_capturedData.begin(), m_capturedData.begin() + frames * 2);
        } else {
            // Underflow: clear output or use what we have
            std::fill(out, out + frames * 2, 0.0f);
        }
    }

    void pushData(const float* data, int samples) {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_capturedData.insert(m_capturedData.end(), data, data + samples);
        
        // Cap size to prevent infinite growth if not processing
        const size_t maxSamples = 44100 * 2; // 1 second buffer
        if (m_capturedData.size() > maxSamples) {
            m_capturedData.erase(m_capturedData.begin(), m_capturedData.begin() + (m_capturedData.size() - maxSamples));
        }
    }

    std::string getName() const override { return "Audio Input"; }
    std::vector<Port> getInputPorts() const override { return {}; }
    std::vector<Port> getOutputPorts() const override { return {{"Stereo Out", 2}}; }

private:
    std::mutex m_bufferMutex;
    std::vector<float> m_capturedData;
};

} // namespace Beam

#endif // INPUT_NODE_HPP





