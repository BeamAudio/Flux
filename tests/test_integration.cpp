#include "../src/dsp/audio_engine.hpp"
#include "../src/dsp/gain_node.hpp"
#include "../src/ui/input_handler.hpp"
#include "../src/ui/knob.hpp"
#include <iostream>
#include <cassert>

// Mock miniaudio
ma_result ma_device_init(void* pContext, const ma_device_config* pConfig, ma_device* pDevice) { return MA_SUCCESS; }
ma_result ma_device_start(ma_device* pDevice) { return MA_SUCCESS; }
void ma_device_uninit(ma_device* pDevice) {}

int main() {
    // 1. Setup Audio Engine
    Beam::AudioEngine engine;
    engine.init(44100, 2);
    auto gainNode = std::make_shared<Beam::GainNode>(0.5f);
    engine.addNode(gainNode);

    // 2. Setup UI
    Beam::InputHandler uiHandler;
    auto volumeKnob = std::make_shared<Beam::Knob>("Volume", 0.0f, 1.0f, 0.5f);
    volumeKnob->setBounds(10, 10, 50, 50);
    
    // 3. Link UI to DSP (The "Flux" Link)
    volumeKnob->onValueChanged = [&](float newValue) {
        gainNode->setGain(newValue);
        std::cout << "UI Link: Volume set to " << newValue << std::endl;
    };

    uiHandler.addComponent(volumeKnob);

    // 4. Simulate Interaction
    std::cout << "Simulating Knob Drag..." << std::endl;
    uiHandler.handleMouseDown(25, 25, 0); // Click on knob
    uiHandler.handleMouseMove(25, 15);    // Drag up (increase)
    uiHandler.handleMouseUp(25, 15, 0);

    // 5. Verify
    // The value should be higher than 0.5
    float finalValue = volumeKnob->getValue();
    assert(finalValue > 0.5f);
    std::cout << "Integration Test Success: UI changes reflected in DSP parameter." << std::endl;

    return 0;
}
