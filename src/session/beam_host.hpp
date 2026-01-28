#ifndef BEAM_HOST_HPP
#define BEAM_HOST_HPP

#include "../engine/audio_engine.hpp"
#include "../render/quad_batcher.hpp"
#include "../render/shader.hpp"
#include "../interface/input_handler.hpp"
#include "flux_project.hpp"
#include "asset_manager.hpp"

namespace Beam {

enum class DAWMode { Splicing, Flux };

class BeamHost {
public:
    BeamHost(const std::string& title, int width, int height);
    ~BeamHost();

    bool init();
    void run();
    void stop();

    void setMode(DAWMode mode);
    DAWMode getMode() const { return m_mode; }

    static void onFileSelected(void* userdata, const char* const* filelist, int filter);

private:
    void handleEvents();
    void update();
    void render();
    void performLayout();

    std::string m_title;
    int m_width;
    int m_height;
    bool m_isRunning;
    DAWMode m_mode = DAWMode::Flux;
    SDL_Window* m_window;
    SDL_GLContext m_glContext;
    
    std::shared_ptr<FluxProject> m_project;
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
    std::shared_ptr<class AudioConfigView> m_configView;
};

} // namespace Beam

#endif // BEAM_HOST_HPP
