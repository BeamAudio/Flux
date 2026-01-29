#ifndef FLUX_NODE_AUDIO_PROCESSOR_WRAPPER_HPP
#define FLUX_NODE_AUDIO_PROCESSOR_WRAPPER_HPP

#include "flux_node.hpp"
#include "../engine/audio_processor.hpp"

namespace Beam {

/**
 * @class FluxNodeAudioProcessorWrapper
 * @brief Wrapper to expose FluxNode as an AudioProcessor for JUCE-like API compatibility
 */
class FluxNodeAudioProcessorWrapper : public AudioProcessor {
public:
    explicit FluxNodeAudioProcessorWrapper(std::shared_ptr<FluxNode> node) 
        : m_fluxNode(node) {
        // Copy parameters from the FluxNode to the AudioProcessor
        auto params = node->getParameters();
        for (const auto& pair : params) {
            addParameter(pair.second);
        }
    }

    // Implement AudioProcessor interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        // Forward to the wrapped node if it has a prepare method
        // For now, we'll just store the values
        m_sampleRate = sampleRate;
        m_blockSize = samplesPerBlock;
    }

    void releaseResources() override {
        // Forward to the wrapped node if needed
    }

    void processBlock(float** audioInputOutput, int numInputChannels, int numOutputChannels, int numSamples, const MIDIBuffer& midiMessages) override {
        // This is tricky - FluxNode expects its own buffer format
        // For now, we'll just call the process method on the node directly
        // This assumes the node's buffers are already set up correctly
        
        // Call the node's process method
        m_fluxNode->process(numSamples);
        
        // If the node has MIDI processing, call it
        m_fluxNode->processMIDI(midiMessages);
    }

    std::string getName() const override {
        return m_fluxNode->getName();
    }

    int getNumInputChannels() const override {
        // This needs to be implemented based on the node's ports
        // For now, return a default value - this should be overridden by specific implementations
        return 2; // Default to stereo
    }

    int getNumOutputChannels() const override {
        // This needs to be implemented based on the node's ports
        // For now, return a default value - this should be overridden by specific implementations
        return 2; // Default to stereo
    }

    // Get the wrapped FluxNode
    std::shared_ptr<FluxNode> getFluxNode() const { return m_fluxNode; }

private:
    std::shared_ptr<FluxNode> m_fluxNode;
};

} // namespace Beam

#endif // FLUX_NODE_AUDIO_PROCESSOR_WRAPPER_HPP





