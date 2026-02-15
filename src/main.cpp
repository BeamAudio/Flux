#define NOMINMAX
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <objbase.h> // For COM
#include "engine/session/beam_host.hpp"
#include <iostream>
#include <exception>
#include <iostream>
#include <fstream>
#include <exception>

int main(int argc, char* argv[]) {
    // Redirect Output to Log File
    std::ofstream logFile("beam_flux.log");
    std::streambuf* coutBuf = std::cout.rdbuf();
    std::streambuf* cerrBuf = std::cerr.rdbuf();
    
    if (logFile.is_open()) {
        std::cout.rdbuf(logFile.rdbuf());
        std::cerr.rdbuf(logFile.rdbuf());
    }

    // Initialize COM (Required for Drag&Drop, VST3, and Native File Dialogs)
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        std::cerr << "Warning: CoInitializeEx failed: " << hr << std::endl;
    }

    try {
        // Initialize the Beam Engine Host
        Beam::BeamHost host("Beam Audio Flux", 1920, 1080);

        std::cout << "Beam Audio Flux Initializing..." << std::endl;

        if (host.init()) {
            std::cout << "Init Successful. Starting Loop." << std::endl;
            host.run();
            std::cout << "Loop Finished Normally." << std::endl;
        } else {
            std::cerr << "Initialization Failed!" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL ERROR (exception): " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "CRITICAL ERROR (unknown)" << std::endl;
    }

    // Restore Buffers
    std::cout.rdbuf(coutBuf);
    std::cerr.rdbuf(cerrBuf);

    CoUninitialize();

    return 0;
}






