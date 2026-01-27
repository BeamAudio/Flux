#ifndef AUDIO_ENGINE_HPP
#define AUDIO_ENGINE_HPP

#include "audio_node.hpp"
#include "miniaudio.h"
#include <vector>
#include <memory>
#include <mutex>

namespace Beam {

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    bool init(int sampleRate, int channels);
    void process(float* output, float* input, int frames);

    void addNode(std::shared_ptr<AudioNode> node);

    void setPlaying(bool playing) { m_isPlaying = playing; }
    bool isPlaying() const { return m_isPlaying; }

    static void dataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

private:
    int m_sampleRate;
    int m_channels;
    std::vector<std::shared_ptr<AudioNode>> m_nodes;
    std::mutex m_nodeMutex;
    ma_device m_device;
    std::atomic<bool> m_isPlaying{false};
};

} // namespace Beam

#endif // AUDIO_ENGINE_HPP
