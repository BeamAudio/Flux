#include "beam_host.hpp"
#include "../dsp/sine_oscillator.hpp"
#include "../dsp/gain_node.hpp"
#include "../ui/knob.hpp"
#include "../graphics/ui_shaders.hpp"
#include <iostream>

namespace Beam {

BeamHost::BeamHost(const std::string& title, int width, int height)
    : m_title(title), m_width(width), m_height(height), m_isRunning(false), 
      m_window(nullptr), m_glContext(nullptr) {
    m_audioEngine = std::make_unique<AudioEngine>();
    m_uiHandler = std::make_unique<InputHandler>();
}

BeamHost::~BeamHost() {
    if (m_glContext) SDL_GL_DestroyContext(m_glContext);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
}

bool BeamHost::init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    m_window = SDL_CreateWindow(m_title.c_str(), m_width, m_height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!m_window) return false;

    m_glContext = SDL_GL_CreateContext(m_window);
    
    if (!gladLoadGLLoader((void*(*)(const char*))SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    
    m_batcher = std::make_unique<QuadBatcher>(5000);
    m_uiShader = std::make_unique<Shader>(UI_VERTEX_SHADER, UI_FRAGMENT_SHADER);

    if (!m_audioEngine->init(44100, 2)) return false;

    // --- SETUP TEST SPLICING DECK ---
    auto gain = std::make_shared<GainNode>(0.5f);
    m_audioEngine->addNode(std::make_shared<SineOscillator>(440.0f, 44100.0f));
    m_audioEngine->addNode(gain);

    auto volumeKnob = std::make_shared<Knob>("Volume", 0.0f, 1.0f, 0.5f);
    volumeKnob->setBounds(100, 100, 40, 100);
    volumeKnob->onValueChanged = [gain](float v) { gain->setGain(v); };
    
    m_uiHandler->addComponent(volumeKnob);

    m_isRunning = true;
    return true;
}

void BeamHost::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            m_isRunning = false;
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            m_uiHandler->handleMouseDown(event.button.x, event.button.y, event.button.button);
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            m_uiHandler->handleMouseUp(event.button.x, event.button.y, event.button.button);
        } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
            m_uiHandler->handleMouseMove(event.motion.x, event.motion.y);
        }
    }
}

void BeamHost::update() {
    // Logic updates go here
}

void BeamHost::render() {
    glClearColor(0.1f, 0.11f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    m_uiShader->use();
    
    float left = 0, right = (float)m_width, bottom = (float)m_height, top = 0;
    float projection[16] = {
        2.0f/(right-left), 0, 0, 0,
        0, 2.0f/(top-bottom), 0, 0,
        0, 0, -1, 0,
        -(right+left)/(right-left), -(top+bottom)/(top-bottom), 0, 1
    };
    m_uiShader->setMat4("projection", projection);

    m_batcher->begin();
    
    // Draw Splicing Deck Background
    m_batcher->drawQuad(50, 50, 300, 400, 0.18f, 0.19f, 0.20f, 1.0f);
    m_batcher->drawQuad(50, 50, 300, 30, 0.25f, 0.5f, 1.0f, 1.0f);

    // Render UI Components (Knobs, etc.)
    m_uiHandler->render(*m_batcher);

    m_batcher->end();
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