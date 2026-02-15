#ifndef BEAM_HOST_HPP
#define BEAM_HOST_HPP

#include "engine/core/audio_engine.hpp"
#include "interface/render/quad_batcher.hpp"
#include "interface/render/shader.hpp"
#include "interface/core/input_handler.hpp"
#include "engine/session/flux_project.hpp"
#include "engine/session/asset_manager.hpp"
#include "engine/session/asset_manager.hpp"
#include "engine/session/app_settings.hpp"
#include <atomic>
#include <mutex>

namespace Beam {

enum class DAWMode { Splicing, Flux, Mix };

class BeamHost {
public:
    BeamHost(const std::string& title, int width, int height);
    ~BeamHost();

    bool init();
    void run();
    void stop();

    void setMode(DAWMode mode);
    DAWMode getMode() const { return m_mode; }
    
    SDL_Window* getWindow() { return m_window; }
    
    AppSettings& getSettings() { return m_settings; }
    void saveSettings() { m_settings.save(); }

    static void onFileSelected(void* userdata, const char* const* filelist, int filter);
    static void onSaveDialogCallback(void* userdata, const char* const* filelist, int filter);
    static void onLoadDialogCallback(void* userdata, const char* const* filelist, int filter);
    static void onRenderDialogCallback(void* userdata, const char* const* filelist, int filter);
    static void onScriptLoadCallback(void* userdata, const char* const* filelist, int filter);

private:
    void handleEvents();
    void update();
    void render(float dt);
    void performLayout();
    void openProjectLoadDialog();
    void processPendingLoad();

    std::string m_title;
    int m_width;
    int m_height;
    bool m_isRunning;
    DAWMode m_mode = DAWMode::Flux;
    SDL_Window* m_window;
    SDL_GLContext m_glContext;
    
    std::shared_ptr<FluxProject> m_project;
    std::string m_currentProjectPath;
    std::string m_pendingLoadPath;
    std::atomic<bool> m_hasPendingLoad{false};
    std::mutex m_loadMutex;
    AppSettings m_settings;
    float m_autosaveTimer = 0.0f;
    std::atomic<bool> m_loadRequested{false};

    std::unique_ptr<AudioEngine> m_audioEngine;
    std::unique_ptr<class AudioDeviceManager> m_audioDeviceManager;
    std::unique_ptr<QuadBatcher> m_batcher;
    std::unique_ptr<Shader> m_uiShader;
    std::unique_ptr<InputHandler> m_uiHandler;
    
    std::shared_ptr<class Workspace> m_workspace;
    std::shared_ptr<class Timeline> m_timeline;
    std::shared_ptr<class TopBar> m_topBar;
    std::shared_ptr<class Sidebar> m_browser;
    std::shared_ptr<class MasterStrip> m_masterStrip;
    std::shared_ptr<class MixerView> m_mixerView;
    std::shared_ptr<class AudioConfigView> m_configView;
    std::shared_ptr<class RenderModal> m_renderModal;
    std::shared_ptr<class ConfirmationModal> m_confirmationModal;
    std::shared_ptr<class SettingsModal> m_settingsModal;
};

} // namespace Beam

#endif // BEAM_HOST_HPP





