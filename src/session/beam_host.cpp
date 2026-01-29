#include "beam_host.hpp"
#include "project_manager.hpp"
#include "../engine/track_node.hpp"
#include "../engine/midi_event.hpp"
#include "../engine/audio_device_manager.hpp"
#include "../interface/workspace.hpp"
#include "../interface/timeline.hpp"
#include "../interface/tape_reel.hpp"
#include "../interface/top_bar.hpp"
#include "../interface/sidebar.hpp"
#include "../interface/master_strip.hpp"
#include "../interface/audio_config_view.hpp"
#include "../render/ui_shaders.hpp"
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

void BeamHost::onSaveDialogCallback(void* userdata, const char* const* filelist, int filter) {
    if (filelist && filelist[0]) {
        BeamHost* host = static_cast<BeamHost*>(userdata);
        if (host && host->m_project) {
            std::string path = filelist[0];
            // Basic extension check
            if (path.length() < 5 || path.substr(path.length() - 5) != ".flux") {
                path += ".flux";
            }
            ProjectManager::saveProject(path, host->m_project->serialize());
            std::cout << "Project saved to: " << path << std::endl;
        }
    }
}

void BeamHost::onLoadDialogCallback(void* userdata, const char* const* filelist, int filter) {
    if (filelist && filelist[0]) {
        BeamHost* host = static_cast<BeamHost*>(userdata);
        auto data = ProjectManager::loadProject(filelist[0]);
        if (!data.empty() && host && host->m_project) {
             host->m_project->deserialize(data);
             std::cout << "Project loaded from: " << filelist[0] << std::endl;
        }
    }
}

BeamHost::BeamHost(const std::string& title, int width, int height)
    : m_title(title), m_width(width), m_height(height), m_isRunning(false), 
      m_window(nullptr), m_glContext(nullptr) {
    m_audioEngine = std::make_unique<AudioEngine>();
    m_audioDeviceManager = std::make_unique<AudioDeviceManager>();
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

    m_window = SDL_CreateWindow(m_title.c_str(), m_width, m_height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
    if (!m_window) return false;

    SDL_ShowWindow(m_window);

    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) return false;

    if (!gladLoadGLLoader((void*(*)(const char*))SDL_GL_GetProcAddress)) return false;
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_batcher = std::make_unique<QuadBatcher>(10000);
    m_uiShader = std::make_unique<Shader>(UI_VERTEX_SHADER, UI_FRAGMENT_SHADER);
    m_batcher->setShader(m_uiShader.get());

    if (!SDL_Init(SDL_INIT_AUDIO)) return false;
    m_audioDeviceManager->initialise(2, 2);
    if (!m_audioEngine->init(44100, 2)) return false;
    
    m_project = std::make_shared<FluxProject>();
    m_audioEngine->setGraph(m_project->getGraph());

    // Add Input Node to the graph so it's ready for capture
    if (m_audioEngine->getInputNode()) {
        m_project->getGraph()->addNode(m_audioEngine->getInputNode());
    }

    m_audioEngine->setPlaying(false); // Start paused
    m_audioEngine->seek(0);           // Start at time 0

    m_workspace = std::make_shared<Workspace>(m_project, m_audioEngine.get());
    m_timeline = std::make_shared<Timeline>(m_project, m_audioEngine.get());
    m_topBar = std::make_shared<TopBar>(m_width);
    m_browser = std::make_shared<Sidebar>(Sidebar::Side::Left);
    m_masterStrip = std::make_shared<MasterStrip>(m_audioEngine->getMasterNode());
    m_configView = std::make_shared<AudioConfigView>(m_audioDeviceManager.get(), m_audioEngine.get());

    m_browser->onAddFX = [this](std::string type) {
        if (m_workspace) {
            m_workspace->addFX(type, 300, 300);
            m_audioEngine->updatePlan();
        }
    };

    m_uiHandler->addComponent(m_workspace);
    m_uiHandler->addComponent(m_timeline);
    m_uiHandler->addComponent(m_browser);
    m_uiHandler->addComponent(m_masterStrip);
    m_uiHandler->addComponent(m_topBar);
    m_uiHandler->addComponent(m_configView);

    m_topBar->onModeChanged = [this](int mode) {
        setMode(mode == 0 ? DAWMode::Flux : DAWMode::Splicing);
    };
    m_topBar->onConfigRequested = [this]() {
        m_configView->setVisible(!m_configView->isVisible());
    };
    m_configView->onConfigChanged = [this]() {
        auto setup = m_audioDeviceManager->getCurrentDeviceSetup();
        std::cout << "Audio Config Changed: " << setup.outputDeviceName << " (" << setup.outputDeviceId << ") @ " << setup.sampleRate << std::endl;
        m_audioEngine->init((int)setup.sampleRate, setup.outputChannels, setup.outputDeviceId, setup.inputDeviceId);
    };
    m_topBar->onPlayRequested = [this]() { m_audioEngine->setPlaying(true); };
    m_topBar->onPauseRequested = [this]() { m_audioEngine->setPlaying(false); };
    m_topBar->onRewindRequested = [this]() { m_audioEngine->rewind(); };
    m_topBar->onRecordRequested = [this](bool recording) {
        if (recording) {
            auto nodes = m_project->getGraph()->getNodes();
            for (auto& [id, node] : nodes) {
                auto track = std::dynamic_pointer_cast<FluxTrackNode>(node);
                if (track) {
                    std::string filename = "recording_" + std::to_string(id) + ".wav";
                    track->startRecording(filename, 44100);
                }
            }
        } else {
            auto nodes = m_project->getGraph()->getNodes();
            for (auto& [id, node] : nodes) {
                auto track = std::dynamic_pointer_cast<FluxTrackNode>(node);
                if (track) track->stopRecording();
            }
        }
    };
    m_topBar->onSaveRequested = [this]() {
        static const SDL_DialogFileFilter filters[] = {
            { "Flux Project", "flux" },
            { "All files", "*" }
        };
        SDL_ShowSaveFileDialog(onSaveDialogCallback, this, m_window, filters, 2, "project.flux");
    };
    m_topBar->onLoadRequested = [this]() {
        static const SDL_DialogFileFilter filters[] = {
            { "Flux Project", "flux" },
            { "All files", "*" }
        };
        SDL_ShowOpenFileDialog(onLoadDialogCallback, this, m_window, filters, 2, NULL, false);
    };

    setMode(DAWMode::Flux);
    performLayout();

    m_isRunning = true;
    return true;
}

void BeamHost::setMode(DAWMode mode) {
    m_mode = mode;
    if (m_mode == DAWMode::Flux) {
        m_workspace->setVisible(true);
        m_timeline->setVisible(false);
        if (m_browser) m_browser->setVisible(true);
    } else {
        m_workspace->setVisible(false);
        m_timeline->setVisible(true);
        if (m_browser) m_browser->setVisible(false);
    }
    performLayout();
}

void BeamHost::performLayout() {
    float topBarHeight = 40.0f;
    float sidebarWidth = (m_mode == DAWMode::Flux) ? 200.0f : 0.0f;
    float masterStripWidth = 120.0f;
    if (m_topBar) m_topBar->setBounds(0, 0, (float)m_width, topBarHeight);
    if (m_browser) m_browser->setBounds(0, topBarHeight, sidebarWidth, (float)m_height - topBarHeight);
    if (m_masterStrip) m_masterStrip->setBounds((float)m_width - masterStripWidth, topBarHeight, masterStripWidth, (float)m_height - topBarHeight);
    if (m_workspace) m_workspace->setBounds(sidebarWidth, topBarHeight, (float)m_width - sidebarWidth - masterStripWidth, (float)m_height - topBarHeight);
    if (m_timeline) m_timeline->setBounds(sidebarWidth, topBarHeight, (float)m_width - sidebarWidth - masterStripWidth, (float)m_height - topBarHeight);
    if (m_configView) m_configView->setBounds(0, 0, (float)m_width, (float)m_height);
}

void BeamHost::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            std::cout << "Quit Event Received." << std::endl;
            m_isRunning = false;
        } 
        else if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_SPACE) {
                bool playing = !m_audioEngine->isPlaying();
                m_audioEngine->setPlaying(playing);
                if (m_topBar) m_topBar->setPlaying(playing);
            }
            if (m_mode == DAWMode::Splicing && m_timeline) {
                m_timeline->handleKeyDown(event.key.key);
            }
        }
        else if (event.type == SDL_EVENT_DROP_FILE) {
            if (m_workspace && event.drop.data) {
                float mx, my; SDL_GetMouseState(&mx, &my);
                m_workspace->addTrack(event.drop.data, mx, my, *m_audioEngine);
                // In SDL3, event strings should be freed if the event owns them.
                // However, SDL_EVENT_DROP_FILE data is managed by SDL in most cases.
                // Let's be safe and check if it crashes without freeing first.
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
        else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
            float mx, my;
            SDL_GetMouseState(&mx, &my);
            m_uiHandler->handleMouseWheel(mx, my, event.wheel.y);
        }
        else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            m_width = event.window.data1;
            m_height = event.window.data2;
            performLayout();
        }
    }
}

void BeamHost::render(float dt) {
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
    m_uiHandler->render(*m_batcher, dt, (float)m_width, (float)m_height);
    m_batcher->flush();
    SDL_GL_SwapWindow(m_window);
}

void BeamHost::run() {
    float audioBuffer[1024 * 2];
    uint64_t lastTime = SDL_GetTicks();
    int heartbeats = 0;
    while (m_isRunning) {
        uint64_t currentTime = SDL_GetTicks();
        float dt = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        handleEvents();
        if (m_uiHandler) m_uiHandler->update(dt);
        MIDIBuffer emptyMidi;
        m_audioEngine->process(audioBuffer, 1024, emptyMidi);
        render(dt);
        heartbeats++;
        if (heartbeats % 500 == 0) std::cout << "DAW Heartbeat: Still alive." << std::endl;
        SDL_Delay(1);
    }
    std::cout << "Main Loop Exited Cleanly." << std::endl;
}

void BeamHost::stop() { m_isRunning = false; }

} // namespace Beam





