#include "beam_host.hpp"
#include "../dsp/track_node.hpp"
#include "../ui/workspace.hpp"
#include "../ui/tape_reel.hpp"
#include "../ui/top_bar.hpp"
#include "../graphics/ui_shaders.hpp"
#include <iostream>
#include <SDL3/SDL_dialog.h>

namespace Beam {

void BeamHost::onFileSelected(void* userdata, const char* const* filelist, int filter) {
    if (filelist && filelist[0]) {
        BeamHost* host = static_cast<BeamHost*>(userdata);
        if (host && host->m_workspace) {
            host->m_workspace->addTrack(filelist[0], 400, 300, *host->m_audioEngine);
        }
    }
}

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
    if (!SDL_Init(SDL_INIT_VIDEO)) return false;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    m_window = SDL_CreateWindow(m_title.c_str(), m_width, m_height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!m_window) return false;

    SDL_ShowWindow(m_window);

    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) return false;

    if (!gladLoadGLLoader((void*(*)(const char*))SDL_GL_GetProcAddress)) return false;
    
    m_batcher = std::make_unique<QuadBatcher>(10000);
    m_uiShader = std::make_unique<Shader>(UI_VERTEX_SHADER, UI_FRAGMENT_SHADER);

    if (!SDL_Init(SDL_INIT_AUDIO)) return false;
    if (!m_audioEngine->init(44100, 2)) return false;
    
    m_audioEngine->setPlaying(true);

    m_workspace = std::make_shared<Workspace>();
    m_uiHandler->addComponent(m_workspace);

    auto topBar = std::make_shared<TopBar>(m_width);
    m_uiHandler->addComponent(topBar);

    m_isRunning = true;
    return true;
}

void BeamHost::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            m_isRunning = false;
        } 
        else if (event.type == SDL_EVENT_DROP_FILE) {
            if (m_workspace) {
                float mx, my;
                SDL_GetMouseState(&mx, &my);
                m_workspace->addTrack(event.drop.data, mx, my, *m_audioEngine);
            }
        }
        else if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_O && (event.key.mod & SDL_KMOD_CTRL)) {
                SDL_ShowOpenFileDialog(onFileSelected, this, m_window, NULL, 0, NULL, false);
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            m_uiHandler->handleMouseDown(event.button.x, event.button.y, event.button.button);
        } 
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            m_uiHandler->handleMouseUp(event.button.x, event.button.y, event.button.button);
        } 
        else if (event.type == SDL_EVENT_MOUSE_MOTION) {
            m_uiHandler->handleMouseMove(event.motion.x, event.motion.y);
        }
        else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            m_width = event.window.data1;
            m_height = event.window.data2;
        }
    }
}

void BeamHost::update() {}

void BeamHost::render() {
    glViewport(0, 0, m_width, m_height);
    glClearColor(0.08f, 0.09f, 0.1f, 1.0f);
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
    m_uiHandler->render(*m_batcher);
    m_batcher->end();

    SDL_GL_SwapWindow(m_window);
}

void BeamHost::run() {
    float audioBuffer[1024 * 2];
    while (m_isRunning) {
        handleEvents();
        update();
        m_audioEngine->process(audioBuffer, 1024);
        render();
    }
}

void BeamHost::stop() {
    m_isRunning = false;
}

} // namespace Beam
