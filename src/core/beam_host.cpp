#include "beam_host.hpp"
#include "../dsp/track_node.hpp"
#include "../ui/workspace.hpp"
#include "../ui/tape_reel.hpp"
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
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return false;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    m_window = SDL_CreateWindow(m_title.c_str(), m_width, m_height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!m_window) return false;

    m_glContext = SDL_GL_CreateContext(m_window);
    gladLoadGLLoader((void*(*)(const char*))SDL_GL_GetProcAddress);
    
    m_batcher = std::make_unique<QuadBatcher>(10000);
    m_uiShader = std::make_unique<Shader>(UI_VERTEX_SHADER, UI_FRAGMENT_SHADER);

    m_audioEngine->init(44100, 2);
    m_audioEngine->setPlaying(true);

    // --- SETUP MEGA-TAPE MIXER ---
    auto workspace = std::make_shared<Workspace>();
    
    // Track 1: Drums
    auto track1 = std::make_shared<TrackNode>("Drums");
    track1->load("drums.wav");
    track1->setState(TrackState::Playing);
    m_audioEngine->addNode(track1);
    workspace->addModule(std::make_shared<TapeReel>("DRUMS", 100, 100, track1));

    // Track 2: Bass
    auto track2 = std::make_shared<TrackNode>("Bass");
    track2->load("bass.wav");
    track2->setState(TrackState::Playing);
    m_audioEngine->addNode(track2);
    workspace->addModule(std::make_shared<TapeReel>("BASS", 100, 250, track2));

    // Track 3: Vocals
    auto track3 = std::make_shared<TrackNode>("Vocals");
    track3->setState(TrackState::Recording);
    m_audioEngine->addNode(track3);
    workspace->addModule(std::make_shared<TapeReel>("VOCALS", 100, 400, track3));

    m_uiHandler->addComponent(workspace);

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
    
    // Render UI Components (Workspace contains TapeReels)
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