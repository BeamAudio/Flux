#ifndef AUDIO_ENGINE_HPP
#define AUDIO_ENGINE_HPP

#include "audio_node.hpp"
#include "flux_graph.hpp"
#include "render_plan.hpp"
#include "master_node.hpp"
#include <SDL3/SDL.h>
#include <vector>
#include <memory>
#include <atomic>

namespace Beam {

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    bool init(int sampleRate, int channels);
    
    // Called from audio thread (or main loop), lock-free
    void process(float* output, int frames);

    // Called from UI thread to update the active processing plan
    void setGraph(std::shared_ptr<FluxGraph> graph);
    void updatePlan();
    
    std::shared_ptr<FluxGraph> getGraph() { return m_graph; }
    std::shared_ptr<MasterNode> getMasterNode() { return m_masterNode; }

    void setPlaying(bool playing);
    bool isPlaying() const { return m_isPlaying; }
    void rewind();

private:
    int m_sampleRate;
    int m_channels;
    
    std::shared_ptr<FluxGraph> m_graph; // "Model" graph (UI thread)
    size_t m_masterNodeId;
    std::shared_ptr<MasterNode> m_masterNode;

    // The active plan used by the audio thread. 
    // Atomic shared_ptr allows lock-free swapping.
    std::atomic<std::shared_ptr<RenderPlan>> m_activePlan;
    
    SDL_AudioStream* m_stream;
    std::atomic<bool> m_isPlaying{false};
};

} // namespace Beam

#endif // AUDIO_ENGINE_HPP