#ifndef BEAM_HOST_HPP
#define BEAM_HOST_HPP

#include <SDL3/SDL.h>
#include <string>
#include <memory>
#include "../dsp/audio_engine.hpp"

namespace Beam {

class BeamHost {
public:
    BeamHost(const std::string& title, int width, int height);
    ~BeamHost();

    bool init();
    void run();
    void stop();

private:
    void handleEvents();
    void update();
    void render();

    std::string m_title;
    int m_width;
    int m_height;
    bool m_isRunning;
    SDL_Window* m_window;
    SDL_GLContext m_glContext;
    std::unique_ptr<AudioEngine> m_audioEngine;
};

} // namespace Beam

#endif // BEAM_HOST_HPP
