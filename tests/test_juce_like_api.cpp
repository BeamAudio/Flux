#include "../src/engine/simple_gain_processor.hpp"
#include "../src/engine/audio_buffer.hpp"
#include "../src/utilities/flux_audio_utils.hpp"
#include <iostream>
#include <memory>

int main() {
    std::cout << "Testing the new JUCE-like API components..." << std::endl;

    // Create a simple gain processor
    auto gainProcessor = std::make_shared<Beam::SimpleGainProcessor>();

    std::cout << "Processor name: " << gainProcessor->getName() << std::endl;
    std::cout << "Initial gain: " << gainProcessor->getGain() << " dB" << std::endl;

    // Create an audio buffer
    Beam::AudioBuffer<float> audioBuffer(2, 512); // 2 channels, 512 samples

    std::cout << "Created audio buffer: " << audioBuffer.getNumChannels()
              << " channels, " << audioBuffer.getNumSamples() << " samples" << std::endl;

    // Test audio utilities
    std::cout << "\nTesting audio utilities:" << std::endl;

    // Apply gain using audio utils
    Beam::AudioUtils::applyGain(audioBuffer, 0.5f);
    std::cout << "Applied gain using AudioUtils" << std::endl;

    // Test conversion utilities
    float linearGain = Beam::AudioUtils::decibelsToLinear(6.0f);
    std::cout << "6 dB in linear: " << linearGain << std::endl;

    float dbGain = Beam::AudioUtils::linearToDecibels(linearGain);
    std::cout << linearGain << " linear in dB: " << dbGain << std::endl;

    std::cout << "\nAll tests completed successfully!" << std::endl;

    return 0;
}

