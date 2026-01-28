#include "simple_gain_processor.hpp"
#include <algorithm>

namespace Beam {

SimpleGainProcessor::SimpleGainProcessor() {
    setupBuffers(1, 1, 1024, 2); // 1 input port (stereo), 1 output port (stereo), 1024 buffer size, 2 channels per frame

    // Create the gain parameter using the value tree state
    m_gainParameter = m_valueTreeState.createAndAddParameter(
        "gain",           // parameter ID
        "Gain",           // parameter name
        "dB",             // parameter label
        -20.0f,           // min value (-20 dB)
        20.0f,            // max value (+20 dB)
        0.0f              // default value (0 dB)
    );

    // Add the parameter to the base class as well
    addParameter(m_gainParameter);
}

SimpleGainProcessor::~SimpleGainProcessor() {
}

void SimpleGainProcessor::process(int frames) {
    // Get the current gain value and convert from dB to linear
    float gainDb = m_gainParameter->getValue();
    float linearGain = std::pow(10.0f, gainDb / 20.0f);

    // Apply gain to the input buffer and put it in the output buffer
    float* input = getInputBuffer(0);
    float* output = getOutputBuffer(0);

    for (int i = 0; i < frames * 2; ++i) { // *2 for stereo
        output[i] = input[i] * linearGain;
    }
}

float SimpleGainProcessor::getGain() const {
    return m_gainParameter->getValue();
}

std::vector<FluxNode::Port> SimpleGainProcessor::getInputPorts() const {
    return { {"Stereo In", 2} };
}

std::vector<FluxNode::Port> SimpleGainProcessor::getOutputPorts() const {
    return { {"Stereo Out", 2} };
}

} // namespace Beam
