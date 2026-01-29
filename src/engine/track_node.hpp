#ifndef TRACK_NODE_HPP
#define TRACK_NODE_HPP

#include "audio_node.hpp"
#include "disk_streamer.hpp"
#include "analog_base.hpp"
#include "../../third_party/dr_wav.h"
#include <string>
#include <atomic>
#include <iostream>
#include <vector>

namespace Beam {

enum class TrackState {
    Idle,
    Playing,
    Recording
};

class TrackNode : public AudioNode {
public:
    TrackNode(const std::string& name) 
        : m_name(name), m_state(TrackState::Idle), m_isWriterOpen(false), 
          m_wowFlutter(44100.0f) 
    {
        m_wowFlutter.setIntensity(0.001f, 0.0005f);
    }

    ~TrackNode() {
        stopRecording();
    }

    void setTapeParams(float drive, float age) {
        m_tapeDrive = drive;
        m_tapeAge = age;
        m_wowFlutter.setIntensity(0.001f * age, 0.002f * age);
    }

    bool load(const std::string& filePath) {
        m_streamer = std::make_unique<DiskStreamer>();
        return m_streamer->open(filePath);
    }

    bool startRecording(const std::string& filePath, int sampleRate, int channels) {
        drwav_data_format format;
        format.container = drwav_container_riff;
        format.format = DR_WAVE_FORMAT_PCM;
        format.channels = (drwav_uint32)channels;
        format.sampleRate = (drwav_uint32)sampleRate;
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

    void process(float* buffer, int frames, int channels, size_t startFrame = 0) override {
        // 1. Capture/Read raw signal
        if (m_state == TrackState::Playing && m_streamer) {
            if (startFrame != m_lastProcessedFrame) m_streamer->seek(startFrame);
            m_streamer->read(buffer, frames, channels);
            m_lastProcessedFrame = startFrame + frames;
        }

        // 2. Apply Tape Physics
        for (int i = 0; i < frames; ++i) {
            float speedMod = m_wowFlutter.next();
            // Saturation and high-end roll-off
            for (int c = 0; c < channels; ++c) {
                float& s = buffer[i * channels + c];
                s = AnalogBase::saturateLangevin(s, 1.0f + m_tapeDrive);
                
                if (m_tapeAge > 0.01f) {
                    m_ageFilters[c].setCutoff(20000.0f - (m_tapeAge * 15000.0f), 44100.0f);
                    s = m_ageFilters[c].process(s);
                }
            }
        }

        // 3. Write if recording
        if (m_state == TrackState::Recording && m_isWriterOpen) {
            std::vector<int16_t> pcm(frames * channels);
            for (int i = 0; i < frames * channels; ++i) {
                float s = buffer[i];
                if (s > 1.0f) s = 1.0f;
                if (s < -1.0f) s = -1.0f;
                pcm[i] = (int16_t)(s * 32767.0f);
            }
            drwav_write_pcm_frames(&m_wavWriter, (drwav_uint64)frames, pcm.data());
        }
    }

    void setState(TrackState state) { 
        m_state = state; 
        if (state == TrackState::Idle) m_lastProcessedFrame = (size_t)-1;
    }
    TrackState getState() const { return m_state; }
    
    void seek(size_t frame) {
        m_lastProcessedFrame = frame;
        if (m_streamer) m_streamer->seek(frame);
    }

    std::vector<std::vector<float>> getPeakData(int numPoints) {
        if (m_streamer) return m_streamer->getPeakData(numPoints);
        return {};
    }

    size_t getTotalFrames() const {
        if (m_streamer) return m_streamer->getTotalFrames();
        return 0;
    }

    std::string getName() const override { return m_name; }

private:
    std::string m_name;
    std::unique_ptr<DiskStreamer> m_streamer;
    std::atomic<TrackState> m_state;
    size_t m_lastProcessedFrame = (size_t)-1;
    
    // Tape Physics
    float m_tapeDrive = 0.0f;
    float m_tapeAge = 0.0f;
    AnalogBase::WowFlutterGenerator m_wowFlutter;
    AnalogBase::OnePoleFilter m_ageFilters[2]; // Stereo age filtering

    drwav m_wavWriter;
    bool m_isWriterOpen;
};

} // namespace Beam

#endif // TRACK_NODE_HPP

