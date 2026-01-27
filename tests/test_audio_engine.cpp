#include "../src/dsp/audio_engine.hpp"
#include "../src/dsp/sine_oscillator.hpp"
#include <iostream>
#include <vector>
#include <cassert>

int main() {
    Beam::AudioEngine engine;
    
    // Test initialization
    bool success = engine.init(44100, 2);
    assert(success);
    engine.setPlaying(true);
    std::cout << "AudioEngine initialized." << std::endl;

    // Test adding a node
    auto osc = std::make_shared<Beam::SineOscillator>(440.0f, 44100.0f);
    engine.addNode(osc);
    std::cout << "Added SineOscillator node." << std::endl;

    // Test processing
    std::vector<float> output(1024 * 2, 0.0f);
    engine.process(output.data(), 1024);

    // Check if we have some non-zero data
    bool hasData = false;
    for (float s : output) {
        if (s != 0.0f) {
            hasData = true;
            break;
        }
    }

    assert(hasData);
    std::cout << "AudioEngine processed frames successfully (non-zero output detected)." << std::endl;

    return 0;
}
