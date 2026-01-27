#include "../src/dsp/audio_engine.hpp"
#include "../src/dsp/sine_oscillator.hpp"
#include "../src/dsp/gain_node.hpp"
#include "../src/dsp/delay_node.hpp"
#include "../src/dsp/biquad_filter_node.hpp"
#include "../src/dsp/wav_writer.hpp"
#include <iostream>
#include <vector>

int main() {
    const int sampleRate = 44100;
    const int channels = 2;
    const int durationSeconds = 3;
    const int totalFrames = sampleRate * durationSeconds;

    Beam::AudioEngine engine;
    engine.init(sampleRate, channels);
    engine.setPlaying(true);

    // Create Chain: Osc -> Filter -> Delay -> Gain
    auto osc = std::make_shared<Beam::SineOscillator>(220.0f, sampleRate); // A3
    auto filter = std::make_shared<Beam::BiquadFilterNode>(Beam::FilterType::LowPass, 1000.0f, 0.707f, sampleRate);
    auto delay = std::make_shared<Beam::DelayNode>(0.25f, 0.4f, sampleRate); // 250ms delay, 40% feedback
    auto gain = std::make_shared<Beam::GainNode>(0.5f);

    engine.addNode(osc);
    engine.addNode(filter);
    engine.addNode(delay);
    engine.addNode(gain);

    std::vector<float> outputBuffer(totalFrames * channels);
    
    // Process in blocks
    const int blockSize = 1024;
    for (int i = 0; i < totalFrames; i += blockSize) {
        int framesToProcess = std::min(blockSize, totalFrames - i);
        engine.process(outputBuffer.data() + (i * channels), framesToProcess);
    }

    if (Beam::WavWriter::write("output.wav", outputBuffer.data(), outputBuffer.size(), sampleRate, channels)) {
        std::cout << "Complex DSP test successful. Generated 'output.wav'" << std::endl;
    } else {
        std::cerr << "Failed to write 'output.wav'" << std::endl;
        return 1;
    }

    return 0;
}
