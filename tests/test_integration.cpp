#include "../src/engine/audio_engine.hpp"
#include "../src/engine/flux_fx_nodes.hpp"
#include "../src/interface/input_handler.hpp"
#include "../src/interface/knob.hpp"
#include <iostream>
#include <cassert>

int main() {
    // 1. Setup Graph and Node
    auto graph = std::make_shared<Beam::FluxGraph>();
    auto gainNode = std::make_shared<Beam::FluxGainNode>(1024);
    size_t nodeId = graph->addNode(gainNode);

    // 2. Setup UI Knob and Bind to Parameter
    auto volumeKnob = std::make_shared<Beam::Knob>("Volume", 0.0f, 1.0f, 0.5f);
    auto gainParam = gainNode->getParameter("Gain");
    assert(gainParam != nullptr);
    
    volumeKnob->bindParameter(gainParam);
    volumeKnob->setBounds(10, 10, 50, 50);
    
    // 3. Setup Input Handler
    Beam::InputHandler uiHandler;
    uiHandler.addComponent(volumeKnob);

    // 4. Simulate Interaction
    std::cout << "Simulating Knob Drag to 0.75..." << std::endl;
    uiHandler.handleMouseDown(25, 25, 0); // Click on knob
    uiHandler.handleMouseMove(25, 10);    // Drag up
    uiHandler.handleMouseUp(25, 10, 0);

    // 5. Verify Synchronization
    float finalUiValue = volumeKnob->getValue();
    float finalDspValue = gainParam->getValue();
    
    std::cout << "UI Value: " << finalUiValue << std::endl;
    std::cout << "DSP Value: " << finalDspValue << std::endl;

    assert(finalUiValue > 0.5f);
    assert(finalUiValue == finalDspValue);
    
    std::cout << "Integration Test Success: UI and DSP parameters are synchronized via the Parameter system." << std::endl;

    return 0;
}



