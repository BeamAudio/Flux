#ifndef INPUT_NODE_HPP
#define INPUT_NODE_HPP

#include "engine/core/flux_node.hpp"
#include "engine/dsp/lock_free_buffer.hpp"
#include <mutex>
#include <vector>

namespace Beam {

/**
 * @class InputNode
 * @brief Provides real-time audio input from the hardware to the Flux Graph.
 */
// Use LockFreeBuffer for high-performance circular buffering
using CaptureBuffer = LockFreeBuffer<float>;

class InputProcessor : public FluxProcessor {
public:
    InputProcessor(std::shared_ptr<CaptureBuffer> buffer, const std::string& deviceId, float gain) 
        : m_buffer(buffer), m_deviceId(deviceId), m_gain(gain) {}

    void process(const float** inputs, float** outputs, int frames) override {
        float* out = outputs[0];
        if (!out) return;

        float currentPeak = 0.0f;
        
        // Read from lock-free buffer directly to output
        size_t needed = frames * 2;
        size_t read = m_buffer->read(out, needed);

        // Apply gain and calculate peak
        for (size_t i = 0; i < read; ++i) {
            out[i] *= m_gain;
            float absS = std::abs(out[i]);
            if (absS > currentPeak) currentPeak = absS;
        }

        // Fill underrun with silence
        if (read < needed) {
            std::fill(out + read, out + needed, 0.0f);
        }

        // Use atomic exchange/store for peak visualization? 
        // We can't store back to 'buffer' struct easily as it's just a typedef now.
        // We need a shared 'peak' atomic somewhere.
        // For simplicity, let's keep peak local or move it to InputNode.
        // Actually, InputNode::getPeakLevel accesses m_peak.
        // We need to store it in a member reachable by InputNode.
        // But InputProcessor is separate.
        // Let's pass a pointer to an atomic peak.
        if (m_peakDest) m_peakDest->store(currentPeak, std::memory_order_relaxed);
    }

    void updateParameters(const float* params) override {
        if (params) {
            m_gain = params[1];
        }
    }
    
    void setPeakDest(std::shared_ptr<std::atomic<float>> peak) { m_peakDest = peak; }

private:
    std::shared_ptr<CaptureBuffer> m_buffer;
    std::string m_deviceId;
    float m_gain = 1.0f;
    std::shared_ptr<std::atomic<float>> m_peakDest;
};

class InputNode : public FluxNode {
public:
    InputNode(int bufferSize) {
        // 1 second buffer (stereo 44.1k)
        m_captureBuffer = std::make_shared<CaptureBuffer>(44100 * 2); 
        m_peakLevel = std::make_shared<std::atomic<float>>(0.0f);
        
        addParameter(std::make_shared<Parameter>("Source", 0.0f, 2.0f, 0.0f));
        addParameter(std::make_shared<Parameter>("Gain", 0.0f, 4.0f, 1.0f)); // 0x to 4x gain (+12dB)
        m_meterSource->addMeter("Level");
    }

    std::shared_ptr<FluxProcessor> createProcessor() override {
        auto proc = std::make_shared<InputProcessor>(m_captureBuffer, m_deviceId, getParameter("Gain")->getValue());
        if (auto p = std::dynamic_pointer_cast<InputProcessor>(proc)) {
            p->setPeakDest(m_peakLevel);
        }
        return proc;
    }

    float getPeakLevel() const { return m_peakLevel->load(std::memory_order_relaxed); }

    void pushData(const float* data, int samples) {
        m_captureBuffer->write(data, samples);
    }

    void setDeviceId(const std::string& id) { m_deviceId = id; }
    std::string getDeviceId() const { return m_deviceId; }

    std::string getName() const override { return "Audio Input"; }
    std::vector<FluxNode::Port> getInputPorts() const override { return {}; }
    std::vector<FluxNode::Port> getOutputPorts() const override { return {{"Stereo Out", 2}}; }

    std::shared_ptr<Component> createEditor(const NodeEditorContext& context) override;

private:
    std::shared_ptr<CaptureBuffer> m_captureBuffer;
    std::shared_ptr<std::atomic<float>> m_peakLevel;
    std::string m_deviceId;
};

} // namespace Beam
#endif // INPUT_NODE_HPP
