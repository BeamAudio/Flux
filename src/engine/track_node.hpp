#ifndef TRACK_NODE_HPP
#define TRACK_NODE_HPP

#include "audio_node.hpp"
#include "disk_streamer.hpp"
#include "../../third_party/dr_wav.h"
#include <string>
#include <atomic>
#include <iostream>

namespace Beam {

enum class TrackState {
    Idle,
    Playing,
    Recording
};

class TrackNode : public AudioNode {
public:
    TrackNode(const std::string& name) : m_name(name), m_state(TrackState::Idle), m_isWriterOpen(false) {}

    ~TrackNode() {
        stopRecording();
    }

    bool load(const std::string& filePath) {
        m_streamer = std::make_unique<DiskStreamer>();
        return m_streamer->open(filePath);
    }

    bool startRecording(const std::string& filePath, int sampleRate, int channels) {
        drwav_data_format format;
        format.container = drwav_container_riff;
        format.format = DR_WAVE_FORMAT_PCM;
        format.channels = channels;
        format.sampleRate = sampleRate;
        format.bitsPerSample = 16;

        if (drwav_init_file_write(&m_wavWriter, filePath.c_str(), &format, NULL)) {
            m_isWriterOpen = true;
            m_state = TrackState::Recording;
            return true;
        }
        return false;
    }

    void stopRecording() {
        if (m_isWriterOpen) {
            drwav_uninit(&m_wavWriter);
            m_isWriterOpen = false;
        }
        m_state = TrackState::Idle;
    }

    void process(float* buffer, int frames, int channels) override {
        // If Recording, the buffer contains the input to be recorded
        if (m_state == TrackState::Recording && m_isWriterOpen) {
            // Write to disk
            std::vector<int16_t> pcm(frames * channels);
            for (int i = 0; i < frames * channels; ++i) {
                float s = buffer[i];
                if (s > 1.0f) s = 1.0f;
                if (s < -1.0f) s = -1.0f;
                pcm[i] = (int16_t)(s * 32767.0f);
            }
            drwav_write_pcm_frames(&m_wavWriter, frames, pcm.data());
        } 
        // If Playing, read from disk into buffer
        else if (m_state == TrackState::Playing && m_streamer) {
            m_streamer->read(buffer, frames, channels);
        }
    }

    void setState(TrackState state) { m_state = state; }
    TrackState getState() const { return m_state; }
    
    void seek(size_t frame) {
        if (m_streamer) m_streamer->seek(frame);
    }

    std::vector<float> getPeakData(int numPoints) {
        if (m_streamer) return m_streamer->getPeakData(numPoints);
        return {};
    }

    std::string getName() const override { return m_name; }

private:
    std::string m_name;
    std::unique_ptr<DiskStreamer> m_streamer;
    std::atomic<TrackState> m_state;
    
    drwav m_wavWriter;
    bool m_isWriterOpen;
};

} // namespace Beam

#endif // TRACK_NODE_HPP

