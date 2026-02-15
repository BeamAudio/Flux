#include "engine/session/beam_host.hpp"
#include "interface/editors/vst_external_editor.hpp"
#include <windows.h>
#include "engine/core/debug_log.hpp"
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
#include "engine/midi/midi_device_manager.hpp" // Added
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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#endif
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_system.h>
#include <SDL3/SDL_video.h>
#include "interface/popups/render_modal.hpp"
#include "interface/popups/confirmation_modal.hpp"
#include "interface/popups/settings_modal.hpp"

// Windows headers for file dialog and COM
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <commdlg.h>
#include <objbase.h>
#endif


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
            host->m_currentProjectPath = path; // Update current path
            std::cout << "Project saved to: " << path << std::endl;
        }
    }
}

void BeamHost::onLoadDialogCallback(void* userdata, const char* const* filelist, int filter) {
    if (filelist && filelist[0]) {
        BeamHost* host = static_cast<BeamHost*>(userdata);
        if (!host || !host->m_project || !host->m_audioEngine) {
            std::cerr << "[Load] Error: Host, project, or engine is null during load callback." << std::endl;
            DebugLog("[Load] Error: Host, project, or engine is null.");
            return;
        }

        std::string path = filelist[0];
        host->m_currentProjectPath = path; // Update current path
        std::cout << "[Load] Starting load from: " << path << std::endl;
        DebugLog("[Load] Starting load from: " + path);

        try {
            auto data = ProjectManager::loadProject(path);
            if (data.empty()) {
                std::cerr << "[Load] Error: Project file is empty or invalid: " << path << std::endl;
                DebugLog("[Load] Error: Project file is empty or invalid.");
                return;
            }

            // CRITICAL: AUDIO-SAFE SHUTDOWN SEQUENCE
            // 1. Mute to prevent new audio from being produced
            DebugLog("[Load] Step 1: Muting engine.");
            host->m_audioEngine->setMuted(true);
            
            // 2. Nullify the active plan so audio callback immediately returns silence
            //    This prevents the audio thread from accessing ANY processor
            DebugLog("[Load] Step 2: Nullifying active plan.");
            host->m_audioEngine->clearActivePlan();
            
            // 3. Wait for any in-progress audio callback to complete
            //    The audio callback sets isProcessing=true at start, false at end
            DebugLog("[Load] Step 3: Waiting for audio callback to finish...");
            int waitCount = 0;
            while (host->m_audioEngine->isProcessing() && waitCount < 100) {
                SDL_Delay(5); // Spin-wait in 5ms increments
                waitCount++;
            }
            if (waitCount >= 100) {
                std::cerr << "[Load] WARNING: Audio callback did not finish in 500ms!" << std::endl;
            }
            DebugLog("[Load] Audio shutdown complete. Safe to destroy nodes.");

            if (host->m_workspace) {
                DebugLog("[Load] Clearing workspace (Start)...");
                std::cout << "[Load] Clearing workspace..." << std::endl;
                host->m_workspace->closeAllVSTEditors(); // Force close windows first
                host->m_workspace->clear();
                std::cout << "[Load] Workspace cleared." << std::endl;
                DebugLog("[Load] Workspace cleared (End).");
            }

            DebugLog("[Load] Deserializing project.");
            host->m_project->deserialize(data);
            DebugLog("[Load] Deserializing complete.");

            // Update Master ID in Engine
            if (host->m_project->getGraph()) {
                size_t masterId = (size_t)-1;
                auto nodes = host->m_project->getGraph()->getNodes();
                for(auto& [id, node] : nodes) {
                    if (node && node->getName() == "Master") {
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

                DebugLog("[Load] Updating engine plan.");
                host->m_audioEngine->updatePlan();
            }

            if (host->m_workspace) {
                host->m_workspace->refresh();
            }
            if (host->m_masterStrip) {
                host->m_masterStrip->refresh();
            }

            host->m_audioEngine->setMuted(false);
            std::cout << "[Load] Project loaded successfully from: " << path << std::endl;
            DebugLog("[Load] Project loaded successfully.");

        } catch (const std::exception& e) {
            std::cerr << "[Load] CRITICAL ERROR during project load: " << e.what() << std::endl;
            DebugLog("[Load] CRITICAL ERROR: " + std::string(e.what()));
            host->m_audioEngine->setMuted(false);
        } catch (...) {
            std::cerr << "[Load] UNKNOWN CRITICAL ERROR during project load." << std::endl;
            DebugLog("[Load] UNKNOWN CRITICAL ERROR.");
            host->m_audioEngine->setMuted(false);
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
    MidiDeviceManager::get().shutdown(); // Shut down MIDI
    SDL_Quit();
}

bool BeamHost::init() {
    if (!SDL_Init(SDL_INIT_VIDEO)) return false;

    SDL_SetHint(SDL_HINT_VIDEO_WAYLAND_ALLOW_LIBDECOR, "0");
    // Prevent flickering by ensuring the main window doesn't try to clear child/owned window areas
    SDL_SetHint(SDL_HINT_VIDEO_WIN_D3DCOMPILER, "none"); 

    m_settings.load(); // Load AppSettings

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

    // DIAGNOSTIC CHANGE: Disable VSync to prevent VST from stalling the swap
    if (!SDL_GL_SetSwapInterval(0)) {
        std::cout << "Warning: Failed to disable VSync: " << SDL_GetError() << std::endl;
    } else {
        std::cout << "VSync Disabled (Immediate Swap) for Freeze Diagnostics." << std::endl;
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
    
    std::cout << "Initializing MIDI..." << std::endl;
    MidiDeviceManager::get().init();
    
    std::cout << "Initializing Device Manager..." << std::endl;
    
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
    
    // Apply Saved Settings if valid
    if (m_settings.audio.sampleRate > 0) {
        initialSetup.sampleRate = m_settings.audio.sampleRate;
        initialSetup.blockSize = m_settings.audio.bufferSize;
        // Optionally set devices if available (skipping for now to avoid invalid device IDs blocking startup)
    }

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

    // Connect MIDI Manager to Audio Engine Input Node
    // We need to find or create the MIDI input node
    std::shared_ptr<MIDIInputNode> midiInputNode;
    if (m_audioEngine->getInputNode()) {
        // Assuming AudioEngine might hold a MIDI node or we find it in the graph?
        // Actually, Workspace creates it.
        // Let's create a global MIDI Input Node if not exists.
    }
    
    // For now, let's just make sure the callback routes to the engine's midi buffer if we have one.
    // AudioEngine doesn't strictly have a "Global MIDI Input" yet, but Workspace might create one.
    // Ideally, we route MIDI to the focused track or a specific MIDI input node.
    // Let's hook it up to the Engine's InputNode if it supports MIDI, or a dedicated one.
    
    MidiDeviceManager::get().setMidiCallback([this](const MIDIEvent& evt) {
        // Route to Active Track or Global MIDI Input
        // For now, we can iterate all MIDIInputNodes in the graph and push events
        if (m_project && m_project->getGraph()) {
            auto nodes = m_project->getGraph()->getNodes();
            for (auto& [id, node] : nodes) {
                auto midiNode = std::dynamic_pointer_cast<MIDIInputNode>(node);
                if (midiNode) {
                    midiNode->pushMIDIEvent(evt);
                }
            }
        }
    });

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
        if (m_topBar->onSettingsRequested) m_topBar->onSettingsRequested();
        if (m_settingsModal) m_settingsModal->setTab(1); // Switch to Audio Tab
    };
    m_topBar->onSettingsRequested = [this]() {
        if (!m_settingsModal) {
            m_settingsModal = std::make_shared<SettingsModal>(this, m_audioDeviceManager.get());
            m_settingsModal->onClose = [this]() { m_settingsModal = nullptr; };
            
            float mx = (float)m_width / 2.0f - 225.0f; // Wider for layout safety
            float my = (float)m_height / 2.0f - 150.0f;
            m_settingsModal->setBounds(mx, my, 450, 300);
        }
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
        try {
            std::cout << "[LOAD] 1. onLoadRequested triggered" << std::endl;
            std::cout.flush();
            
            // 1. HARD Mute engine - Stop all audio processing immediately
            std::cout << "[LOAD] 1a. About to mute engine" << std::endl;
            std::cout.flush();
            if (m_audioEngine) m_audioEngine->setMuted(true);
            std::cout << "[LOAD] 1b. Engine muted" << std::endl;
            std::cout.flush();
            
            // 2. Dirty Check
            bool isDirty = m_project && m_project->isDirty();
            std::cout << "[LOAD] 2. isDirty = " << (isDirty ? "true" : "false") << std::endl;
            std::cout.flush();
            
            if (isDirty) {
                std::cout << "[LOAD] 3. About to create confirmation modal" << std::endl;
                std::cout.flush();
                
                m_confirmationModal = std::make_shared<ConfirmationModal>("Discard unsaved changes and load project?");
                std::cout << "[LOAD] 3a. Modal created" << std::endl;
                std::cout.flush();
                
                m_confirmationModal->onSave = [this]() {
                    m_topBar->onSaveRequested(); // Save first
                    m_confirmationModal = nullptr; 
                };
                m_confirmationModal->onDiscard = [this]() {
                    m_confirmationModal = nullptr;
                    m_loadRequested = true; // DEFER to next frame
                };
                m_confirmationModal->onCancel = [this]() { 
                    m_confirmationModal = nullptr; 
                    if (m_audioEngine) m_audioEngine->setMuted(false); // Restore audio if cancel
                };
                
                float mx = (float)m_width / 2.0f - 200.0f;
                float my = (float)m_height / 2.0f - 80.0f;
                std::cout << "[LOAD] 3b. About to setBounds" << std::endl;
                std::cout.flush();
                m_confirmationModal->setBounds(mx, my, 400, 160);
                std::cout << "[LOAD] 3c. Modal fully initialized" << std::endl;
                std::cout.flush();
            } else {
                // Not dirty, defer to next frame
                std::cout << "[LOAD] 4. Project clean, setting m_loadRequested = true" << std::endl;
                std::cout.flush();
                m_loadRequested = true;
                std::cout << "[LOAD] 4a. m_loadRequested set" << std::endl;
                std::cout.flush();
            }
            std::cout << "[LOAD] 5. Callback completed successfully" << std::endl;
            std::cout.flush();
        } catch (const std::exception& e) {
            std::cerr << "[LOAD] EXCEPTION: " << e.what() << std::endl;
            std::cerr.flush();
        } catch (...) {
            std::cerr << "[LOAD] UNKNOWN EXCEPTION" << std::endl;
            std::cerr.flush();
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

void BeamHost::openProjectLoadDialog() {
    std::cout << "[LOAD] Opening Project Load Dialog" << std::endl;
    #ifdef _WIN32
    char filename[MAX_PATH] = {0};
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    ofn.lpstrFilter = "Flux Project (*.flux)\0*.flux\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    // Release mouse capture before opening dialog
    SDL_CaptureMouse(false);

    if (GetOpenFileNameA(&ofn)) {
        std::vector<const char*> fileList = { filename, nullptr };
        onLoadDialogCallback(this, fileList.data(), 0);
    } else {
        std::cout << "[LOAD] User canceled native dialog." << std::endl;
        if (m_audioEngine) m_audioEngine->setMuted(false); // Restore audio
    }
    #else
    static const SDL_DialogFileFilter filters[] = { { "Flux Project", "flux" }, { "All files", "*" } };
    SDL_ShowOpenFileDialog(onLoadDialogCallback, this, m_window, filters, 2, NULL, false);
    #endif
}

void BeamHost::processPendingLoad() {
    if (!m_loadRequested) return;
    
    std::cout << "[LOAD] Processing deferred load request (Safe Dialog Mode)" << std::endl;
    
    // CRITICAL FIX: Do NOT destroy VST windows here - it corrupts the COM state
    // and causes GetOpenFileNameA to crash. Instead, just HIDE them.
    if (m_workspace) {
        std::cout << "[LOAD] Step A: Hiding VST windows (not destroying)..." << std::endl;
        m_workspace->hideAllVSTEditors();
    }
    
    // Audio engine is already muted from onLoadRequested
    
    // Reset flag and open dialog - VST cleanup happens in onLoadDialogCallback
    m_loadRequested = false;
    openProjectLoadDialog();
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
    // Process Pending Project Load (from Dialog Thread)
    if (m_hasPendingLoad) {
        m_hasPendingLoad = false;
        std::string path;
        {
            std::lock_guard<std::mutex> lock(m_loadMutex);
            path = m_pendingLoadPath;
        }
        const char* files[] = { path.c_str(), nullptr };
        onLoadDialogCallback(this, files, 0);
    }

    // 1. Process VST Message Pumps (Limited)
    VSTExternalEditor::processMessageLoop();

    // 2. Standard SDL Event Loop (with safety break to prevent floods from freezing the host)
    SDL_Event event;
    int eventCount = 0;
    while (SDL_PollEvent(&event)) {
        eventCount++;
        if (eventCount > 200) break; // Don't get stuck forever if VST floods

        if (event.type == SDL_EVENT_QUIT) {
            m_isRunning = false; 
            return;
        }
        
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            if (event.window.windowID == SDL_GetWindowID(m_window)) {
                // Only exit if main DAW window is closed
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
                    m_isRunning = false;
                }
            } else {
                // For VST windows, we just hide the window via SDL
                SDL_Window* win = SDL_GetWindowFromID(event.window.windowID);
                if (win) SDL_HideWindow(win);
            }
            // Continue processing other events
            continue; 
        }

        if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
            bool isDown = (event.type == SDL_EVENT_KEY_DOWN);
            if (isDown && event.key.repeat) {
                // Ignore repeat for MIDI
            } else {
                int note = -1;
                switch(event.key.key) {
                    case SDLK_Z: note = 48; break; // C3
                    case SDLK_S: note = 49; break; // C#3
                    case SDLK_X: note = 50; break; // D3
                    case SDLK_D: note = 51; break; // D#3
                    case SDLK_C: note = 52; break; // E3
                    case SDLK_V: note = 53; break; // F3
                    case SDLK_G: note = 54; break; // F#3
                    case SDLK_B: note = 55; break; // G3
                    case SDLK_H: note = 56; break; // G#3
                    case SDLK_N: note = 57; break; // A3
                    case SDLK_J: note = 58; break; // A#3
                    case SDLK_M: note = 59; break; // B3
                    case SDLK_COMMA: note = 60; break; // C4
                }
                
                if (note != -1) {
                    // Send to Active Track or Global MIDI
                    MIDIEvent evt;
                    evt.frameOffset = 0;
                    evt.status = isDown ? 0x90 : 0x80;
                    evt.data1 = (uint8_t)note;
                    evt.data2 = isDown ? 100 : 0;
                    evt.timestamp = 0.0;
                    
                    // Route to graph
                    if (m_project && m_project->getGraph()) {
                        auto nodes = m_project->getGraph()->getNodes();
                        for (auto& [id, node] : nodes) {
                            auto midiNode = std::dynamic_pointer_cast<MIDIInputNode>(node);
                            if (midiNode) midiNode->pushMIDIEvent(evt);
                        }
                    }
                }
            }

            // Existing Key Down Logic
            if (isDown) {
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
                if (m_mode == DAWMode::Flux && m_workspace) {
                    m_workspace->handleKeyDown(event.key.key);
                }
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
            
            if (m_renderModal) {
                if (m_renderModal->onMouseDown(event.button.x, event.button.y, event.button.button, shift)) {
                    std::cout << "[BeamHost] MouseDown consumed by RenderModal" << std::endl;
                    modalConsumed = true;
                }
            }
            
            if (!modalConsumed && m_settingsModal) {
                if (m_settingsModal->onMouseDown(event.button.x, event.button.y, event.button.button, shift)) {
                    modalConsumed = true;
                }
            }
            
            if (!modalConsumed && m_confirmationModal) {
                if (m_confirmationModal->onMouseDown(event.button.x, event.button.y, event.button.button, shift)) {
                    std::cout << "[BeamHost] MouseDown consumed by ConfirmationModal" << std::endl;
                    modalConsumed = true;
                }
            }
            
            if (!modalConsumed)
                m_uiHandler->handleMouseDown(event.button.x, event.button.y, event.button.button, shift);
            
            std::cout.flush();
        } 
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            bool shift = (SDL_GetModState() & SDL_KMOD_SHIFT);
            bool modalConsumed = false;
            if (m_renderModal && m_renderModal->onMouseUp(event.button.x, event.button.y, event.button.button, shift)) modalConsumed = true;
            else if (m_settingsModal && m_settingsModal->onMouseUp(event.button.x, event.button.y, event.button.button, shift)) modalConsumed = true;
            else if (m_confirmationModal && m_confirmationModal->onMouseUp(event.button.x, event.button.y, event.button.button, shift)) modalConsumed = true;
            
            if (!modalConsumed)
                m_uiHandler->handleMouseUp(event.button.x, event.button.y, event.button.button, shift);
        } 
        else if (event.type == SDL_EVENT_MOUSE_MOTION) {
            bool shift = (SDL_GetModState() & SDL_KMOD_SHIFT);
            bool modalConsumed = false;
            if (m_renderModal && m_renderModal->onMouseMove(event.motion.x, event.motion.y, shift)) modalConsumed = true;
            else if (m_settingsModal && m_settingsModal->onMouseMove(event.motion.x, event.motion.y, shift)) modalConsumed = true;
            else if (m_confirmationModal && m_confirmationModal->onMouseMove(event.motion.x, event.motion.y, shift)) modalConsumed = true;
            
            if (!modalConsumed)
                m_uiHandler->handleMouseMove(event.motion.x, event.motion.y, shift);
        }
        else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
            float mx, my;
            SDL_GetMouseState(&mx, &my);
            bool modalConsumed = false;
            if (m_renderModal && m_renderModal->onMouseWheel(mx, my, event.wheel.y, (SDL_GetModState() & SDL_KMOD_SHIFT))) modalConsumed = true;
            else if (m_settingsModal && m_settingsModal->onMouseWheel(mx, my, event.wheel.y, (SDL_GetModState() & SDL_KMOD_SHIFT))) modalConsumed = true;
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
    if (!m_window || !m_glContext) return;
    
    // ENSURE OUR CONTEXT IS CURRENT (Fixes flickering when plugins use OpenGL)
    SDL_GL_MakeCurrent(m_window, m_glContext);

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
    
        // PROTECT HOST STATE FROM VST CORRUPTION
        m_batcher->saveState();

        m_uiHandler->render(*m_batcher, dt, (float)m_width, (float)m_height);
        
        if (m_renderModal) {
            // Draw Dimmer
            m_batcher->drawQuad(0.0f, 0.0f, (float)m_width, (float)m_height, 0.0f, 0.0f, 0.0f, 0.5f);
            m_renderModal->render(*m_batcher, dt, (float)m_width, (float)m_height);
        }

        if (m_settingsModal) {
            // Draw Dimmer
            m_batcher->drawQuad(0.0f, 0.0f, (float)m_width, (float)m_height, 0.0f, 0.0f, 0.0f, 0.5f);
            m_settingsModal->render(*m_batcher, dt, (float)m_width, (float)m_height);
        }

        if (m_confirmationModal) {
            // Draw Dimmer
            m_batcher->drawQuad(0.0f, 0.0f, (float)m_width, (float)m_height, 0.0f, 0.0f, 0.0f, 0.5f);
            m_confirmationModal->render(*m_batcher, dt, (float)m_width, (float)m_height);
        }

        // --- Hover Tooltip Overlay ---
        if (m_uiHandler) {
            Component* hovered = m_uiHandler->getHoveredComponent();
            if (hovered) {
                std::string tip = hovered->getTooltipText();
                if (!tip.empty()) {
                    float tw = AudioUtils::calculateTextWidth(tip, 10) + 10.0f;
                    float th = 18.0f;
                    float mx, my; SDL_GetMouseState(&mx, &my);
                    float tx = mx + 15.0f;
                    float ty = my + 15.0f;
                    
                    // Keep on screen
                    if (tx + tw > (float)m_width) tx = mx - tw - 5.0f;
                    if (ty + th > (float)m_height) ty = my - th - 5.0f;

                    m_batcher->drawRoundedRect(tx, ty, tw, th, 4.0f, 0.5f, 0.05f, 0.05f, 0.07f, 0.9f);
                    m_batcher->drawRect(tx, ty, tw, th, 1.0f, Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b, 0.4f);
                    m_batcher->drawVectorText(tip, tx + 5, ty + 3, 10, 1.0f, 1.0f, 1.0f, 1.0f);
                }
            }
        }

        m_batcher->restoreState();

    // --- RENDER HEARTBEAT (Freeze Diagnostic) ---
    // Flashes a small square in the bottom-right corner.
    // If this stops flashing, render() is not being called.
    static float heartbeatTimer = 0.0f;
    heartbeatTimer += dt * 10.0f; // Fast flash
    float alpha = (std::sin(heartbeatTimer) + 1.0f) * 0.5f;
    m_batcher->drawRect((float)m_width - 20, (float)m_height - 20, 10, 10, 1.0f, 1.0f, 0.0f, 0.2f, alpha); // Magenta Heatbeat

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
    
            // AUTOSAVE LOGIC
            if (m_settings.autosaveEnabled && !m_currentProjectPath.empty()) {
                m_autosaveTimer += dt;
                if (m_autosaveTimer > m_settings.autosaveIntervalMinutes * 60.0f) {
                    m_autosaveTimer = 0.0f;
                    if (m_project && m_project->isDirty()) {
                        std::cout << "[Autosave] Saving project to " << m_currentProjectPath << "..." << std::endl;
                        if (m_workspace) m_workspace->saveStateToProject();
                        ProjectManager::saveProject(m_currentProjectPath, m_project->serialize());
                    }
                }
            }

            ParameterQueue::get().dispatch();
            AsyncCallbackQueue::get().dispatch();
            GarbageCollector::get().collect();
            
            processPendingLoad(); // Handle hardened loading protocol
            
            handleEvents();
        if (m_renderModal) m_renderModal->update(dt);
        if (m_settingsModal) m_settingsModal->update(dt);
        if (m_confirmationModal) m_confirmationModal->update(dt);
        if (m_uiHandler && !m_renderModal && !m_confirmationModal && !m_settingsModal) m_uiHandler->update(dt);
        render(dt);
        if (heartbeats % 60 == 0) {
            std::cout << "[BeamHost] Heartbeat: " << heartbeats << " Time: " << dt << std::endl;
        }
        heartbeats++;
        SDL_Delay(8); // Cap at ~120 FPS if VSync is disabled
    }
    std::cout << "Main Loop Exited Cleanly." << std::endl;
}

void BeamHost::stop() { m_isRunning = false; }

} // namespace Beam