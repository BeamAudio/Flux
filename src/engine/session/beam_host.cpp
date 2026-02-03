#include "engine/session/beam_host.hpp"
#include "interface/core/theme.hpp"
#include "engine/session/project_manager.hpp"
#include "engine/session/parameter_queue.hpp"
#include "engine/session/undo_manager.hpp"
#include "engine/plugins/plugin_library.hpp"
#include "engine/dsp/async_callback_queue.hpp"
#include "engine/core/flux_compiler.hpp"
#include "engine/dsp/garbage_collector.hpp"
#include "engine/nodes/track_node.hpp"
#include "engine/nodes/midi_track_node.hpp"
#include "engine/nodes/midi_input_node.hpp"
#include "engine/midi/midi_event.hpp"
#include "engine/core/audio_device_manager.hpp"
#include "engine/core/offline_renderer.hpp"
#include "interface/views/workspace.hpp"
#include "interface/views/timeline.hpp"
#include "interface/views/tape_reel.hpp"
#include "interface/views/top_bar.hpp"
#include "interface/views/sidebar.hpp"
#include "interface/views/master_strip.hpp"
#include "interface/views/mixer_view.hpp"
#include "interface/views/audio_config_view.hpp"
#include "interface/render/ui_shaders.hpp"
#include "interface/core/layout.hpp"
#include "interface/core/coordinate_system.hpp"
#include <iostream>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_system.h>
#include <SDL3/SDL_video.h>
#include "interface/popups/render_modal.hpp"
#include "interface/popups/confirmation_modal.hpp"


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
            if (path.length() < 5 || path.substr(path.length() - 5) != ".flux") {
                path += ".flux";
            }
            if (host->m_workspace) host->m_workspace->saveStateToProject();
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
             
             // Update Master ID in Engine
             if (host->m_project->getGraph()) {
                 size_t masterId = (size_t)-1;
                 for(auto& [id, node] : host->m_project->getGraph()->getNodes()) {
                     if (node->getName() == "Master") {
                         masterId = id;
                         break;
                     }
                 }
                 
                 // Failsafe: If no Master node found (e.g. old project or corruption), create one
                 if (masterId == (size_t)-1) {
                     std::cout << "[Load] Warning: No Master Node found in project. Creating one." << std::endl;
                     masterId = host->m_project->getGraph()->addNode(host->m_audioEngine->getMasterNode());
                 }

                 if (masterId != (size_t)-1) {
                     host->m_audioEngine->setMasterNodeId(masterId);
                     std::cout << "[Load] Master Node ID updated to: " << masterId << std::endl;
                 }
                 
                 host->m_audioEngine->updatePlan();
             }

             if (host->m_workspace) {
                 host->m_workspace->clear();
                 host->m_workspace->refresh();
             }
             if (host->m_masterStrip) {
                 host->m_masterStrip->refresh();
                 std::cout << "[Load] MasterStrip refreshed." << std::endl;
             }
             std::cout << "Project loaded from: " << filelist[0] << std::endl;
        }
    }
}

void BeamHost::onRenderDialogCallback(void* userdata, const char* const* filelist, int filter) {
    std::cout << "[BeamHost] onRenderDialogCallback triggered." << std::endl;
    std::cout.flush();
    if (filelist && filelist[0]) {
        BeamHost* host = static_cast<BeamHost*>(userdata);
        if (host && host->m_project) {
            std::string path = filelist[0];
            std::cout << "[BeamHost] Target path: " << path << std::endl;
            std::cout.flush();
            if (path.length() < 4 || path.substr(path.length() - 4) != ".wav") path += ".wav";
            
            size_t maxFrame = 0;
            std::cout << "[BeamHost] Calculating max frame..." << std::endl;
            std::cout.flush();
            for(auto& t : host->m_project->getTracks()) {
                for(auto& r : t.regions) {
                    size_t end = r.startFrame + r.duration;
                    if (end > maxFrame) maxFrame = end;
                }
            }
            if (maxFrame == 0) maxFrame = 44100 * 5; 
            std::cout << "[BeamHost] Max frame: " << maxFrame << std::endl;
            std::cout.flush();
            
            std::cout << "[BeamHost] Suspending Audio Engine..." << std::endl;
            std::cout.flush();
            host->m_audioEngine->setPlaying(false);
            host->m_audioEngine->setMuted(true); // SUSPEND REAL-TIME DSP
            SDL_Delay(50); // Safety wait for thread exit
            
            auto plan = host->m_audioEngine->getActivePlan();
            if (!plan) {
                std::cerr << "[BeamHost] No active plan to render!" << std::endl;
                host->m_audioEngine->setMuted(false);
                return;
            }

            // Create Modal
            std::cout << "[BeamHost] Creating RenderModal..." << std::endl;
            std::cout.flush();
            try {
                host->m_renderModal = std::make_shared<RenderModal>(path, plan, maxFrame);
                std::cout << "[BeamHost] RenderModal created and assigned." << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[BeamHost] CRASH during RenderModal creation: " << e.what() << std::endl;
                host->m_audioEngine->setMuted(false);
            } catch (...) {
                std::cerr << "[BeamHost] UNKNOWN CRASH during RenderModal creation" << std::endl;
                host->m_audioEngine->setMuted(false);
            }
            std::cout.flush();
            
            host->m_renderModal->onClose = [host]() { 
                std::cout << "[BeamHost] RenderModal closing." << std::endl;
                std::cout.flush();
                host->m_renderModal = nullptr; 
                host->m_audioEngine->setMuted(false); // RESUME REAL-TIME DSP
            };
            
            // Center it (approximate)
            float mx = (float)host->m_width / 2.0f - 200.0f;
            float my = (float)host->m_height / 2.0f - 100.0f;
            host->m_renderModal->setBounds(mx, my, 400, 200);
        }
    } else {
        std::cout << "[BeamHost] Render dialog canceled or no file selected." << std::endl;
        std::cout.flush();
    }
}

void BeamHost::onScriptLoadCallback(void* userdata, const char* const* filelist, int filter) {
    if (filelist && filelist[0]) {
        BeamHost* host = static_cast<BeamHost*>(userdata);
        if (host && host->m_workspace) {
            host->m_workspace->addScriptFX(filelist[0], 300, 300);
            host->m_audioEngine->updatePlan();
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
    if (!m_glContext) {
        std::cerr << "CRITICAL: SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
        return false;
    }

    if (!SDL_GL_SetSwapInterval(1)) {
        std::cout << "Warning: Failed to set VSync: " << SDL_GetError() << std::endl;
    } else {
        std::cout << "VSync Enabled." << std::endl;
    }

    // Load Icon
    SDL_Surface* icon = SDL_LoadBMP("assets/images/FLUX_LOGO.bmp");
    if (!icon) icon = SDL_LoadBMP("../assets/images/FLUX_LOGO.bmp");
    if (!icon) icon = SDL_LoadBMP("../../assets/images/FLUX_LOGO.bmp");

    if (icon) {
        SDL_SetWindowIcon(m_window, icon);
        SDL_DestroySurface(icon);
    } else {
        std::cout << "Warning: FLUX_LOGO.bmp not found for window icon. Error: " << SDL_GetError() << std::endl;
    }

    if (!gladLoadGLLoader((void*(*)(const char*))SDL_GL_GetProcAddress)) {
        std::cerr << "CRITICAL: gladLoadGLLoader failed!" << std::endl;
        return false;
    }
    
    std::cout << "OpenGL Loaded Successfully." << std::endl;
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::cout << "Creating QuadBatcher..." << std::endl;
    m_batcher = std::make_unique<QuadBatcher>(10000);
    std::cout << "Creating UI Shader..." << std::endl;
    m_uiShader = std::make_unique<Shader>(UI_VERTEX_SHADER, UI_FRAGMENT_SHADER);
    m_batcher->setShader(m_uiShader.get());

    std::cout << "Initializing SDL Audio..." << std::endl;
    if (!SDL_Init(SDL_INIT_AUDIO)) {
        std::cerr << "CRITICAL: SDL_Init(AUDIO) failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    std::cout << "Initializing Device Manager..." << std::endl;
    // Removed redundant direct initialise() call, rely on first startAudio if needed 
    // or just let it use its defaults.
    
    m_audioDeviceManager->onConfigChanged = [this]() {
        auto setup = m_audioDeviceManager->getCurrentDeviceSetup();
        std::cout << "Global Audio Config Change: " << setup.sampleRate << "Hz, " << setup.blockSize << " samples." << std::endl;
        m_audioEngine->init((int)setup.sampleRate, setup.outputChannels, setup.blockSize, setup.outputDeviceId, setup.inputDeviceId);
    };

    // Connect Engine to Device Manager BEFORE starting
    m_audioDeviceManager->setAudioCallback([this](const std::map<std::string, float*>& in, float** out, int f, int inc, int outc, double sr) {
        m_audioEngine->audioCallback(in, out, f, inc, outc, sr);
    });

    auto initialSetup = m_audioDeviceManager->getCurrentDeviceSetup();
    std::cout << "Initializing Audio Engine..." << std::endl;
    m_audioEngine->init((int)initialSetup.sampleRate, initialSetup.outputChannels, initialSetup.blockSize, initialSetup.outputDeviceId, initialSetup.inputDeviceId);

    // Start Audio
    std::cout << "Starting Audio Stream..." << std::endl;
    if (m_audioDeviceManager->startAudio() != 0) {
        std::cerr << "CRITICAL: Failed to start audio stream!" << std::endl;
    } else {
        std::cout << "Audio Stream Started Successfully." << std::endl;
    }

    // Scan for Plugins
    for (const auto& path : PluginLibrary::get().getScanPaths()) {
        if (path == "plugins") FluxCompiler::scanUserPlugins();
        else FluxCompiler::scanVST3(path);
    }
    
    std::cout << "Loading Project..." << std::endl;
    m_project = std::make_shared<FluxProject>();
    m_audioEngine->setGraph(m_project->getGraph());

    // Register Master in Graph if not present
    size_t masterId = (size_t)-1;
    for(auto& [id, node] : m_project->getGraph()->getNodes()) {
        if (node->getName() == "Master") { masterId = id; break; }
    }
    if (masterId == (size_t)-1) {
        masterId = m_project->getGraph()->addNode(m_audioEngine->getMasterNode());
        m_audioEngine->setMasterNodeId(masterId);
    }

    // Add Input Node to the graph so it's ready for capture
    if (m_audioEngine->getInputNode()) {
        m_project->getGraph()->addNode(m_audioEngine->getInputNode());
    }

    m_audioEngine->setPlaying(false); // Start paused
    m_audioEngine->seek(0);           // Start at time 0

    std::cout << "Creating UI Components..." << std::endl;
    std::cout << " - Workspace" << std::endl;
    
    void* nativeHWND = nullptr;
#ifdef _WIN32
    nativeHWND = SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#endif

    m_workspace = std::make_shared<Workspace>(m_project, m_audioEngine.get(), m_audioDeviceManager.get(), nativeHWND);
    std::cout << " - Timeline" << std::endl;
    m_timeline = std::make_shared<Timeline>(m_project, m_audioEngine.get());
    std::cout << " - TopBar" << std::endl;
    m_topBar = std::make_shared<TopBar>(this);
    std::cout << " - Sidebar" << std::endl;
    m_browser = std::make_shared<Sidebar>(this, Sidebar::Side::Left);
    std::cout << " - MasterStrip" << std::endl;
    m_masterStrip = std::make_shared<MasterStrip>(m_audioEngine.get());
    std::cout << " - MixerView" << std::endl;
    m_mixerView = std::make_shared<MixerView>(m_project, m_audioEngine.get());
    std::cout << " - ConfigView" << std::endl;
    m_configView = std::make_shared<AudioConfigView>(this, m_audioDeviceManager.get(), m_audioEngine.get());

    m_browser->onAddFX = [this](std::string type) {
        if (type == "Script") {
            static const SDL_DialogFileFilter filters[] = { { "FluxScript", "fluxscript" } };
            SDL_ShowOpenFileDialog(onScriptLoadCallback, this, m_window, filters, 1, NULL, false);
            return;
        }
        if (m_workspace) {
            m_workspace->addFX(type, 300, 300);
            m_audioEngine->updatePlan();
        }
    };

    m_uiHandler->addComponent(m_workspace);
    m_uiHandler->addComponent(m_timeline);
    m_uiHandler->addComponent(m_mixerView);
    m_uiHandler->addComponent(m_browser);
    m_uiHandler->addComponent(m_masterStrip);
    m_uiHandler->addComponent(m_topBar);
    m_uiHandler->addComponent(m_configView);

    m_topBar->onModeChanged = [this](int mode) {
        // Redundant as setDAWMode calls setMode, but safe if TopBar calls it.
        // Actually, to avoid recursion if we added it there:
    };
    m_topBar->onConfigRequested = [this]() {
        m_configView->setVisible(!m_configView->isVisible());
    };
    m_topBar->onUndoRequested = [this]() { UndoManager::get().undo(); };
    m_topBar->onRedoRequested = [this]() { UndoManager::get().redo(); };
    m_topBar->onPlayRequested = [this]() { m_audioEngine->setPlaying(true); };
    m_topBar->onPauseRequested = [this]() { m_audioEngine->setPlaying(false); };
    m_topBar->onRewindRequested = [this]() { m_audioEngine->rewind(); };
    m_topBar->onRecordRequested = [this](bool recording) {
        if (recording) {
            auto nodes = m_project->getGraph()->getNodes();
            for (auto& [id, node] : nodes) {
                // Audio Recording
                auto track = std::dynamic_pointer_cast<FluxTrackNode>(node);
                if (track) {
                    std::string filename = "recording_" + std::to_string(id) + ".wav";
                    track->startRecording(filename, 44100);
                }
                // MIDI Recording
                auto midiTrack = std::dynamic_pointer_cast<MIDITrackNode>(node);
                if (midiTrack) {
                    midiTrack->startRecording();
                }
            }
        } else {
            auto nodes = m_project->getGraph()->getNodes();
            for (auto& [id, node] : nodes) {
                auto track = std::dynamic_pointer_cast<FluxTrackNode>(node);
                if (track) track->stopRecording();
                
                auto midiTrack = std::dynamic_pointer_cast<MIDITrackNode>(node);
                if (midiTrack) midiTrack->stopRecording();
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
        if (m_project && m_project->isDirty()) {
            m_confirmationModal = std::make_shared<ConfirmationModal>("Discard unsaved changes and load project?");
            m_confirmationModal->onSave = [this]() {
                m_topBar->onSaveRequested();
                m_confirmationModal = nullptr; 
            };
            m_confirmationModal->onDiscard = [this]() {
                m_confirmationModal = nullptr;
                static const SDL_DialogFileFilter filters[] = { { "Flux Project", "flux" }, { "All files", "*" } };
                SDL_ShowOpenFileDialog(onLoadDialogCallback, this, m_window, filters, 2, NULL, false);
            };
            m_confirmationModal->onCancel = [this]() { m_confirmationModal = nullptr; };
            
            float mx = (float)m_width / 2.0f - 200.0f;
            float my = (float)m_height / 2.0f - 80.0f;
            m_confirmationModal->setBounds(mx, my, 400, 160);
        } else {
            static const SDL_DialogFileFilter filters[] = { { "Flux Project", "flux" }, { "All files", "*" } };
            SDL_ShowOpenFileDialog(onLoadDialogCallback, this, m_window, filters, 2, NULL, false);
        }
    };
    m_topBar->onRenderRequested = [this]() {
        static const SDL_DialogFileFilter filters[] = { { "WAV Audio", "wav" } };
        SDL_ShowSaveFileDialog(onRenderDialogCallback, this, m_window, filters, 1, "output.wav");
    };
    m_topBar->onScriptRequested = [this]() {
        static const SDL_DialogFileFilter filters[] = { { "Flux Script", "fluxscript" } };
        SDL_ShowOpenFileDialog(onScriptLoadCallback, this, m_window, filters, 1, NULL, false);
    };
    m_topBar->onToolSelected = [this](int toolIdx) {
        if (m_timeline) {
            m_timeline->setTool((TimelineTool)toolIdx);
        }
    };

    setMode(DAWMode::Flux);
    performLayout();

    m_isRunning = true;
    return true;
}

void BeamHost::setMode(DAWMode mode) {
    m_mode = mode;
    
    // Update TopBar mode indicator (0=Flux, 1=Slice, 2=Mix)
    if (m_topBar) {
        int modeIdx = (mode == DAWMode::Flux) ? 0 : ((mode == DAWMode::Splicing) ? 1 : 2);
        m_topBar->setDAWMode(modeIdx);
    }
    
    // Hide all main views first
    if (m_workspace) m_workspace->setVisible(false);
    if (m_timeline) m_timeline->setVisible(false);
    if (m_mixerView) m_mixerView->setVisible(false);
    if (m_masterStrip) m_masterStrip->setVisible(false);
    
    switch (m_mode) {
        case DAWMode::Flux:
            m_workspace->setVisible(true);
            m_masterStrip->setVisible(true);
            if (m_browser) {
                 m_browser->setVisible(true);
                 m_browser->setMode(Sidebar::Mode::Browser);
            }
            break;
            
        case DAWMode::Splicing:
            m_timeline->setVisible(true);
            m_masterStrip->setVisible(true);
            if (m_browser) {
                 m_browser->setVisible(true);
                 m_browser->setMode(Sidebar::Mode::Inspector);
            }
            break;
            
        case DAWMode::Mix:
            m_mixerView->setVisible(true);
            m_mixerView->refresh(); // Rebuild channel strips
            if (m_browser) {
                 m_browser->setVisible(false); // No browser in Mix mode
            }
            break;
    }
    performLayout();
}

void BeamHost::performLayout() {
    // Root Layout (Vertical: TopBar | Content)
    FlexBox rootLayout;
    rootLayout.flexDirection(FlexBox::Direction::Column);

    // 1. TopBar (Fixed Height)
    if (m_topBar) {
        rootLayout.addItem(LayoutItem(m_topBar.get()).withFixedSize(m_width, 40));
    }

    // 2. Content Area (Flexible)
    // We don't have a single container component for the content, so we calculate the content bounds first.
    // However, FlexBox expects items to layout. 
    // Strategy: Use FlexBox to calculate the rect for the "Content" area, 
    // then use a second FlexBox for the horizontal layout within that rect.
    
    // Actually, since BeamHost isn't a component, we can just run the layout on a dummy item or just use the math.
    // But let's just use the FlexBox for the horizontal part directly, knowing the TopBar is 40px.
    
    // --- Global Chassis Layout ---
    // Top Deck: Header Unit
    Rect topDeckBounds = {0, 0, (float)m_width, 60.0f}; // 60px Rack Unit Height
    if (m_topBar) {
        m_topBar->setBounds(topDeckBounds);
    }

    // Main Body: Everything below Top Deck
    Rect bodyBounds = {0, topDeckBounds.h, (float)m_width, (float)m_height - topDeckBounds.h};

    FlexBox bodyLayout;
    bodyLayout.flexDirection(FlexBox::Direction::Row)
              .flexWrap(false) // Never wrap the main columns
              .justifyContent(FlexBox::JustifyContent::FlexStart);

    // 1. Left Wing: Inspector / Browser (Context Aware)
    // Always present in the layout structure, but content changes
    if (m_browser) {
        // In Flux Mode: It's the "Parts Bin"
        // In Slice Mode: It's the "Track Inspector" (reuse for now)
        bool showLeftPanel = true; // Always show for now (Studio Look)
        if (showLeftPanel) {
             bodyLayout.addItem(LayoutItem(m_browser.get()).withWidth(220)); // Fixed Hardware Width
        } else {
             m_browser->setVisible(false);
        }
    }

    // 2. Center Stage: The Mutable View
    // This fills the remaining space
    Component* activeView = nullptr;
    if (m_mode == DAWMode::Flux) {
        activeView = m_workspace.get();
        if (m_timeline) m_timeline->setVisible(false);
        if (m_mixerView) m_mixerView->setVisible(false);
        if (m_workspace) m_workspace->setVisible(true);
    } else if (m_mode == DAWMode::Splicing) {
        activeView = m_timeline.get();
        if (m_workspace) m_workspace->setVisible(false);
        if (m_mixerView) m_mixerView->setVisible(false);
        if (m_timeline) m_timeline->setVisible(true);
    } else if (m_mode == DAWMode::Mix) {
        activeView = m_mixerView.get();
        if (m_workspace) m_workspace->setVisible(false);
        if (m_timeline) m_timeline->setVisible(false);
        if (m_mixerView) m_mixerView->setVisible(true);
    }

    if (activeView) {
        bodyLayout.addItem(LayoutItem(activeView).withFlex(1.0f)); 
    }

    // 3. Right Wing: Master Section (NOT shown in Mix mode - it's part of MixerView)
    if (m_masterStrip && m_mode != DAWMode::Mix) {
        m_masterStrip->setVisible(true);
        bodyLayout.addItem(LayoutItem(m_masterStrip.get()).withWidth(140)); // Slightly wider for meters
    } else if (m_masterStrip) {
        m_masterStrip->setVisible(false);
    }

    bodyLayout.performLayout(bodyBounds);

    if (m_configView) {
        m_configView->setBounds(0, 0, (float)m_width, (float)m_height);
    }
}

void BeamHost::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            if (m_project && m_project->isDirty()) {
                m_confirmationModal = std::make_shared<ConfirmationModal>("Save changes before exiting?");
                m_confirmationModal->onSave = [this]() {
                    m_topBar->onSaveRequested();
                    m_confirmationModal = nullptr;
                };
                m_confirmationModal->onDiscard = [this]() {
                    m_isRunning = false;
                };
                m_confirmationModal->onCancel = [this]() {
                    m_confirmationModal = nullptr;
                };
                float mx = (float)m_width / 2.0f - 200.0f;
                float my = (float)m_height / 2.0f - 80.0f;
                m_confirmationModal->setBounds(mx, my, 400, 160);
            } else {
                std::cout << "Quit Event Received." << std::endl;
                m_isRunning = false;
            }
        } 
        else if (event.type == SDL_EVENT_KEY_DOWN) {
            bool ctrl = (SDL_GetModState() & SDL_KMOD_CTRL);
            if (ctrl && event.key.key == SDLK_Z) {
                UndoManager::get().undo();
            }
            if (ctrl && event.key.key == SDLK_Y) {
                UndoManager::get().redo();
            }

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
            bool shift = (SDL_GetModState() & SDL_KMOD_SHIFT);
            bool modalConsumed = false;
            if (m_renderModal && m_renderModal->onMouseDown(event.button.x, event.button.y, event.button.button, shift)) modalConsumed = true;
            else if (m_confirmationModal && m_confirmationModal->onMouseDown(event.button.x, event.button.y, event.button.button, shift)) modalConsumed = true;
            
            if (!modalConsumed)
                m_uiHandler->handleMouseDown(event.button.x, event.button.y, event.button.button, shift);
        } 
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            bool shift = (SDL_GetModState() & SDL_KMOD_SHIFT);
            bool modalConsumed = false;
            if (m_renderModal && m_renderModal->onMouseUp(event.button.x, event.button.y, event.button.button, shift)) modalConsumed = true;
            else if (m_confirmationModal && m_confirmationModal->onMouseUp(event.button.x, event.button.y, event.button.button, shift)) modalConsumed = true;
            
            if (!modalConsumed)
                m_uiHandler->handleMouseUp(event.button.x, event.button.y, event.button.button, shift);
        } 
        else if (event.type == SDL_EVENT_MOUSE_MOTION) {
            bool shift = (SDL_GetModState() & SDL_KMOD_SHIFT);
            bool modalConsumed = false;
            if (m_renderModal && m_renderModal->onMouseMove(event.motion.x, event.motion.y, shift)) modalConsumed = true;
            else if (m_confirmationModal && m_confirmationModal->onMouseMove(event.motion.x, event.motion.y, shift)) modalConsumed = true;
            
            if (!modalConsumed)
                m_uiHandler->handleMouseMove(event.motion.x, event.motion.y, shift);
        }
        else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
            float mx, my;
            SDL_GetMouseState(&mx, &my);
            bool modalConsumed = false;
            if (m_renderModal && m_renderModal->onMouseWheel(mx, my, event.wheel.y, (SDL_GetModState() & SDL_KMOD_SHIFT))) modalConsumed = true;
            else if (m_confirmationModal && m_confirmationModal->onMouseWheel(mx, my, event.wheel.y, (SDL_GetModState() & SDL_KMOD_SHIFT))) modalConsumed = true;
            
            if (!modalConsumed)
                m_uiHandler->handleMouseWheel(mx, my, event.wheel.y);
        }
        else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            m_width = event.window.data1;
            m_height = event.window.data2;
            CoordinateSystem::get().setScreenDimensions(m_width, m_height);
            performLayout();
        }
    }
}

void BeamHost::render(float dt) {
    glViewport(0, 0, m_width, m_height);
    // Fill background with "Bakelite" color
    glClearColor(Theme::Bakelite.r, Theme::Bakelite.g, Theme::Bakelite.b, 1.0f); 
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
        m_batcher->resetViewTransform((float)m_width, (float)m_height);
    
        m_uiHandler->render(*m_batcher, dt, (float)m_width, (float)m_height);
        
        if (m_renderModal) {
            // Draw Dimmer
            m_batcher->drawQuad(0.0f, 0.0f, (float)m_width, (float)m_height, 0.0f, 0.0f, 0.0f, 0.5f);
            m_renderModal->render(*m_batcher, dt, (float)m_width, (float)m_height);
        }

        if (m_confirmationModal) {
            // Draw Dimmer
            m_batcher->drawQuad(0.0f, 0.0f, (float)m_width, (float)m_height, 0.0f, 0.0f, 0.0f, 0.5f);
            m_confirmationModal->render(*m_batcher, dt, (float)m_width, (float)m_height);
        }
    m_batcher->flush();
    SDL_GL_SwapWindow(m_window);
}

void BeamHost::run() {
    uint64_t lastTime = SDL_GetTicks();
    int heartbeats = 0;
        while (m_isRunning) {
            uint64_t currentTime = SDL_GetTicks();
            float dt = (currentTime - lastTime) / 1000.0f;
            lastTime = currentTime;
    
            ParameterQueue::get().dispatch();
            AsyncCallbackQueue::get().dispatch();
            GarbageCollector::get().collect();
            handleEvents();
        if (m_renderModal) m_renderModal->update(dt);
        if (m_confirmationModal) m_confirmationModal->update(dt);
        if (m_uiHandler && !m_renderModal && !m_confirmationModal) m_uiHandler->update(dt);
        render(dt);
        heartbeats++;
        if (heartbeats % 500 == 0) std::cout << "DAW Heartbeat: Still alive." << std::endl;
        SDL_Delay(8); // Cap at ~120 FPS if VSync is disabled
    }
    std::cout << "Main Loop Exited Cleanly." << std::endl;
}

void BeamHost::stop() { m_isRunning = false; }

} // namespace Beam





