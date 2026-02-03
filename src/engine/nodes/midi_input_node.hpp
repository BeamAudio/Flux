#ifndef MIDI_INPUT_NODE_HPP
#define MIDI_INPUT_NODE_HPP

#include "engine/core/flux_node.hpp"
#include <mutex>
#include <vector>

namespace Beam {

/**
 * @class MIDIInputNode
 * @brief Provides real-time MIDI input from the hardware to the Flux Graph.
 */
class MIDIInputProcessor : public FluxProcessor {
public:
    MIDIInputProcessor(std::shared_ptr<MIDIBuffer> buffer) : m_sharedBuffer(buffer) {}

    void process(const float** inputs, float** outputs, int frames) override {
        // MIDI Input nodes don't process audio, just pass-through if connected
        if (inputs[0] && outputs[0]) {
            memcpy(outputs[0], inputs[0], frames * 2 * sizeof(float));
        }
    }

    void processMIDI(MIDIBuffer& midi) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& event : m_sharedBuffer->getEvents()) {
            midi.addEvent(event);
        }
        m_sharedBuffer->clear();
    }

private:
    std::shared_ptr<MIDIBuffer> m_sharedBuffer;
    std::mutex m_mutex;
};

class MIDIInputNode : public FluxNode {
public:
    MIDIInputNode() {
        m_buffer = std::make_shared<MIDIBuffer>();
    }

    std::shared_ptr<FluxProcessor> createProcessor() override {
        return std::make_shared<MIDIInputProcessor>(m_buffer);
    }

    void pushMIDIEvent(const MIDIEvent& event) {
        m_buffer->addEvent(event);
    }

    std::string getName() const override { return "MIDI Input"; }
    std::vector<FluxNode::Port> getInputPorts() const override { return {}; }
    std::vector<FluxNode::Port> getOutputPorts() const override { return {{"MIDI Out", 0}}; }

private:
    std::shared_ptr<MIDIBuffer> m_buffer;
};

} // namespace Beam
#endif // MIDI_INPUT_NODE_HPP
