#include "beam_host.hpp"
#include "../dsp/sine_oscillator.hpp"
#include <iostream>

namespace Beam {

BeamHost::BeamHost(const std::string& title, int width, int height)
    : m_title(title), m_width(width), m_height(height), m_isRunning(false), m_window(nullptr), m_glContext(nullptr) {
    m_audioEngine = std::make_unique<AudioEngine>();
}

BeamHost::~BeamHost() {
    if (m_glContext) SDL_GL_DeleteContext(m_glContext);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
}

bool BeamHost::init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    m_window = SDL_CreateWindow(m_title.c_str(), m_width, m_height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!m_window) {
        std::cerr << "Window Creation Error: " << SDL_GetError() << std::endl;
        return false;
    }

    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) {
        std::cerr << "GL Context Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Initialize Audio Engine
    if (!m_audioEngine->init(44100, 2)) {
        std::cerr << "Audio Engine Initialization Failed." << std::endl;
        return false;
    }

    // Add a test oscillator
    m_audioEngine->addNode(std::make_shared<SineOscillator>(440.0f, 44100.0f));

    m_isRunning = true;
    return true;
}

void BeamHost::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            m_isRunning = false;
        }
    }
}

void BeamHost::update() {
    // Logic updates go here
}

void BeamHost::render() {
    // Render commands go here
    SDL_GL_SwapWindow(m_window);
}

void BeamHost::run() {
    while (m_isRunning) {
        handleEvents();
        update();
        render();
    }
}

void BeamHost::stop() {
    m_isRunning = false;
}

} // namespace Beam
