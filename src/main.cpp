#define NOMINMAX
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include "engine/session/beam_host.hpp"
#include <iostream>
#include <exception>
#include <iostream>
#include <exception>

int main(int argc, char* argv[]) {
    try {
        // Initialize the Beam Engine Host
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






