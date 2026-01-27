#ifndef AUDIO_ENGINE_HPP
#define AUDIO_ENGINE_HPP

#include "audio_node.hpp"
#include "flux_graph.hpp"
#include "master_node.hpp"
#include <SDL3/SDL.h>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

namespace Beam {

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    bool init(int sampleRate, int channels);
    void process(float* output, int frames);

    void setGraph(std::shared_ptr<FluxGraph> graph);
    std::shared_ptr<FluxGraph> getGraph() { return m_graph; }
    std::shared_ptr<MasterNode> getMasterNode() { return m_masterNode; }

    void setPlaying(bool playing) { m_isPlaying = playing; }
    bool isPlaying() const { return m_isPlaying; }
    void rewind();

private:
    int m_sampleRate;
    int m_channels;
    
    std::shared_ptr<FluxGraph> m_graph;
    size_t m_masterNodeId;
    std::shared_ptr<MasterNode> m_masterNode;

    std::mutex m_engineMutex;
    
    SDL_AudioStream* m_stream;
    std::atomic<bool> m_isPlaying{false};
};

} // namespace Beam

#endif // AUDIO_ENGINE_HPP