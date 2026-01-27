#ifndef BEAM_HOST_HPP
#define BEAM_HOST_HPP

#include "../dsp/audio_engine.hpp"
#include "../graphics/quad_batcher.hpp"
#include "../graphics/shader.hpp"
#include "../ui/input_handler.hpp"

namespace Beam {

class BeamHost {
public:
    BeamHost(const std::string& title, int width, int height);
    ~BeamHost();

    bool init();
    void run();
    void stop();

    static void onFileSelected(void* userdata, const char* const* filelist, int filter);

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
    std::unique_ptr<QuadBatcher> m_batcher;
    std::unique_ptr<Shader> m_uiShader;
    std::unique_ptr<InputHandler> m_uiHandler;
    std::shared_ptr<class Workspace> m_workspace;
};

} // namespace Beam

#endif // BEAM_HOST_HPP
