#ifndef INPUT_NODE_HPP
#define INPUT_NODE_HPP

#include "engine/core/flux_node.hpp"
#include <mutex>
#include <vector>

namespace Beam {

/**
 * @class InputNode
 * @brief Provides real-time audio input from the hardware to the Flux Graph.
 */
struct CaptureBuffer {
    std::mutex mutex;
    std::vector<float> data;
    std::atomic<float> peak;
    
    void push(const float* source, int samples) {
        std::lock_guard<std::mutex> lock(mutex);
        data.insert(data.end(), source, source + samples);
        
        // Limit to 1 second
        const size_t maxS = 44100 * 2;
        if (data.size() > maxS) {
            data.erase(data.begin(), data.begin() + (data.size() - maxS));
        }
    }
};

class InputProcessor : public FluxProcessor {
public:
    InputProcessor(std::shared_ptr<CaptureBuffer> buffer, const std::string& deviceId, float gain) 
        : m_buffer(buffer), m_deviceId(deviceId), m_gain(gain) {}

    void process(const float** inputs, float** outputs, int frames) override {
        float* out = outputs[0];
        float currentPeak = 0.0f;
        
        std::lock_guard<std::mutex> lock(m_buffer->mutex);
        
        if (m_buffer->data.size() >= (size_t)(frames * 2)) {
            for(int i = 0; i < frames * 2; ++i) {
                float s = m_buffer->data[i] * m_gain; // Apply Gain
                float absS = std::abs(s);
                if (absS > currentPeak) currentPeak = absS;
                out[i] = s;
            }
            m_buffer->data.erase(m_buffer->data.begin(), m_buffer->data.begin() + frames * 2);
        } else {
            std::fill(out, out + frames * 2, 0.0f);
        }

        float prev = m_buffer->peak.load();
        if (currentPeak < prev) currentPeak = prev * 0.92f; 
        m_buffer->peak.store(currentPeak);
    }

    void updateParameters(const float* params) override {
        if (params) {
            // Index 0 is Source (Capture Device selection handled elsewhere or ignored here)
            // Index 1 is Gain
            m_gain = params[1];
        }
    }

private:
    std::shared_ptr<CaptureBuffer> m_buffer;
    std::string m_deviceId;
    float m_gain = 1.0f;
};

class InputNode : public FluxNode {
public:
    InputNode(int bufferSize) {
        m_captureBuffer = std::make_shared<CaptureBuffer>();
        addParameter(std::make_shared<Parameter>("Source", 0.0f, 2.0f, 0.0f));
        addParameter(std::make_shared<Parameter>("Gain", 0.0f, 4.0f, 1.0f)); // 0x to 4x gain (+12dB)
        m_meterSource->addMeter("Level");
    }

    std::shared_ptr<FluxProcessor> createProcessor() override {
        return std::make_shared<InputProcessor>(m_captureBuffer, m_deviceId, getParameter("Gain")->getValue());
    }

    float getPeakLevel() const { return m_captureBuffer->peak.load(); }

    void pushData(const float* data, int samples) {
        m_captureBuffer->push(data, samples);
    }

    void setDeviceId(const std::string& id) { m_deviceId = id; }
    std::string getDeviceId() const { return m_deviceId; }

    std::string getName() const override { return "Audio Input"; }
    std::vector<FluxNode::Port> getInputPorts() const override { return {}; }
    std::vector<FluxNode::Port> getOutputPorts() const override { return {{"Stereo Out", 2}}; }

    std::shared_ptr<Component> createEditor(const NodeEditorContext& context) override;

private:
    std::shared_ptr<CaptureBuffer> m_captureBuffer;
    std::string m_deviceId;
};

} // namespace Beam
#endif // INPUT_NODE_HPP
