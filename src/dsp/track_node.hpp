#ifndef TRACK_NODE_HPP
#define TRACK_NODE_HPP

#include "audio_node.hpp"
#include "disk_streamer.hpp"
#include <string>
#include <atomic>

namespace Beam {

enum class TrackState {
    Idle,
    Playing,
    Recording
};

class TrackNode : public AudioNode {
public:
    TrackNode(const std::string& name) : m_name(name), m_state(TrackState::Idle) {}

    bool load(const std::string& filePath) {
        m_streamer = std::make_unique<DiskStreamer>();
        return m_streamer->open(filePath);
    }

    void process(float* buffer, int frames, int channels) override {
        if (m_state == TrackState::Playing && m_streamer) {
            m_streamer->read(buffer, frames, channels);
        } else if (m_state == TrackState::Recording) {
            // In a real scenario, we'd capture from pInput and write to disk
            // For now, we'll just clear the buffer to represent monitoring
        }
    }

    void setState(TrackState state) { m_state = state; }
    TrackState getState() const { return m_state; }
    
    void seek(size_t frame) {
        if (m_streamer) m_streamer->seek(frame);
    }

    std::string getName() const override { return m_name; }

private:
    std::string m_name;
    std::unique_ptr<DiskStreamer> m_streamer;
    std::atomic<TrackState> m_state;
};

} // namespace Beam

#endif // TRACK_NODE_HPP
