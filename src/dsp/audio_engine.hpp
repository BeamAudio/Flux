#ifndef AUDIO_ENGINE_HPP
#define AUDIO_ENGINE_HPP

#include "audio_node.hpp"
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

    void addNode(std::shared_ptr<AudioNode> node);

    void setPlaying(bool playing) { m_isPlaying = playing; }
    bool isPlaying() const { return m_isPlaying; }

private:
    int m_sampleRate;
    int m_channels;
    std::vector<std::shared_ptr<AudioNode>> m_nodes;
    std::mutex m_nodeMutex;
    
    SDL_AudioStream* m_stream;
    std::atomic<bool> m_isPlaying{false};
};

} // namespace Beam

#endif // AUDIO_ENGINE_HPP