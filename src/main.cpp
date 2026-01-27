#include "core/beam_host.hpp"

int main(int argc, char* argv[]) {
    Beam::BeamHost host("Beam Audio Flux", 1280, 720);

    if (host.init()) {
        host.run();
    }

    return 0;
}