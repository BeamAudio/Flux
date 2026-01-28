#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include "session/beam_host.hpp"
#include "engine/simple_gain_processor.hpp"
#include "interface/slider.hpp"
#include "interface/button.hpp"
#include "engine/audio_device_manager.hpp"
#include <iostream>
#include <exception>

int main(int argc, char* argv[]) {
    try {
        // Example of using the new JUCE-like API components
        std::cout << "Creating a simple gain processor..." << std::endl;

        auto gainProcessor = std::make_shared<Beam::SimpleGainProcessor>();
        // gainProcessor->prepareToPlay(44100.0, 512); // This method doesn't exist in FluxNode

        std::cout << "Processor name: " << gainProcessor->getName() << std::endl;
        std::cout << "Created simple gain processor successfully" << std::endl;

        // Example of using the audio device manager
        Beam::AudioDeviceManager deviceManager;
        deviceManager.initialise(2, 2); // 2 input, 2 output channels

        auto outputDevices = deviceManager.getAvailableOutputDevices();
        std::cout << "Available output devices: " << outputDevices.size() << std::endl;

        auto inputDevices = deviceManager.getAvailableInputDevices();
        std::cout << "Available input devices: " << inputDevices.size() << std::endl;

        // Now run the main application
        Beam::BeamHost host("Beam Audio Flux", 1280, 720);

        std::cout << "Beam Audio Flux Initializing..." << std::endl;

        if (host.init()) {
            std::cout << "Init Successful. Starting Loop." << std::endl;
            host.run();
            std::cout << "Loop Finished Normally." << std::endl;
        } else {
            std::cerr << "Initialization Failed!" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL ERROR (exception): " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "CRITICAL ERROR (unknown)" << std::endl;
        return 1;
    }

    return 0;
}

