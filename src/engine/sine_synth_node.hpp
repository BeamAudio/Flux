#ifndef SINE_SYNTH_NODE_HPP
#define SINE_SYNTH_NODE_HPP

#include "flux_node.hpp"
#include <cmath>
#include <iostream>

namespace Beam {

/**
 * @class SineSynthNode
 * @brief A basic monophonic sine wave synthesizer that responds to MIDI.
 */
class SineSynthNode : public FluxNode {
public:
    SineSynthNode(int bufferSize, float sampleRate) 
        : m_sampleRate(sampleRate), m_phase(0.0f), m_active(false) {
        setupBuffers(0, 1, bufferSize, 2);
    }

    void processMIDI(const MIDIBuffer& midi) override {
        for (const auto& event : midi.getEvents()) {
            uint8_t type = event.status & 0xF0;
            if (type == 0x90 && event.data2 > 0) { // Note On
                m_frequency = 440.0f * std::pow(2.0f, (float)(event.data1 - 69) / 12.0f);
                m_active = true;
                // Note: Simplified monophonic, no envelope yet
            } else if (type == 0x80 || (type == 0x90 && event.data2 == 0)) { // Note Off
                m_active = false;
            }
        }
    }

    void process(int frames) override {
        float* out = getOutputBuffer(0);
        float phaseIncr = (2.0f * 3.1415926535f * m_frequency) / m_sampleRate;

        for (int i = 0; i < frames; ++i) {
            float val = m_active ? std::sin(m_phase) * 0.5f : 0.0f;
            out[i * 2 + 0] = val; // Left
            out[i * 2 + 1] = val; // Right
            
            m_phase += phaseIncr;
            if (m_phase > 2.0f * 3.1415926535f) m_phase -= 2.0f * 3.1415926535f;
        }
    }

    std::string getName() const override { return "Sine Synth"; }
    
    std::vector<Port> getInputPorts() const override { return {}; }
    std::vector<Port> getOutputPorts() const override {
        return { {"Stereo Out", 2} };
    }

private:
    float m_sampleRate;
    float m_frequency = 440.0f;
    float m_phase;
    bool m_active;
};

} // namespace Beam

#endif // SINE_SYNTH_NODE_HPP



