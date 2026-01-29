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
    enum class CaptureMode { AudioMono, AudioStereo, MIDI };

    InputNode(int bufferSize) : m_mode(CaptureMode::AudioStereo), m_peak(0.0f) {
        setupBuffers(0, 1, bufferSize, 2); 
        addParameter(std::make_shared<Parameter>("Capture Mode", 0.0f, 2.0f, 1.0f)); // 0: Mono, 1: Stereo, 2: MIDI
    }

    void process(int frames) override {
        float* out = getOutputBuffer(0);
        float currentPeak = 0.0f;
        
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        if (m_capturedData.size() >= (size_t)(frames * 2)) {
            for(int i=0; i<frames*2; ++i) {
                float s = m_capturedData[i];
                if (std::abs(s) > currentPeak) currentPeak = std::abs(s);
                out[i] = s;
            }
            m_capturedData.erase(m_capturedData.begin(), m_capturedData.begin() + frames * 2);
        } else {
            std::fill(out, out + frames * 2, 0.0f);
        }

        float prev = m_peak.load();
        if (currentPeak < prev) currentPeak = prev * 0.95f; 
        m_peak.store(currentPeak);
    }

    float getPeakLevel() const { return m_peak.load(); }

    void pushData(const float* data, int samples) {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_capturedData.insert(m_capturedData.end(), data, data + samples);
        const size_t maxSamples = 44100 * 2;
        if (m_capturedData.size() > maxSamples) {
            m_capturedData.erase(m_capturedData.begin(), m_capturedData.begin() + (m_capturedData.size() - maxSamples));
        }
    }

    std::string getName() const override { return "Audio Input"; }
    std::vector<FluxNode::Port> getInputPorts() const override { return {}; }
    std::vector<FluxNode::Port> getOutputPorts() const override { return {{"Stereo Out", 2}}; }

private:
    std::mutex m_bufferMutex;
    std::vector<float> m_capturedData;
    CaptureMode m_mode;
    std::atomic<float> m_peak;
};

} // namespace Beam

#endif // INPUT_NODE_HPP





