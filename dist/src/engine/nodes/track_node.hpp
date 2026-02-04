#ifndef TRACK_NODE_HPP
#define TRACK_NODE_HPP

#include "engine/core/audio_node.hpp"
#include "engine/io/disk_streamer.hpp"
#include "engine/nodes/analog_base.hpp"
#include "engine/dsp/lock_free_buffer.hpp"
#include "dr_wav.h"
#include <string>
#include <atomic>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

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
          m_wowFlutter(44100.0f), m_stopDiskThread(false), m_stopPlaybackThread(false)
    {
        m_wowFlutter.setIntensity(0.001f, 0.0005f);
        m_recordBuffer = std::make_unique<LockFreeBuffer<float>>(44100 * 4); // 1 second stereo
        m_playbackBuffer = std::make_unique<LockFreeBuffer<float>>(44100 * 8); // 2 seconds stereo
        
        m_seekTarget.store(-1);
    }

    ~TrackNode() {
        stopRecording();
        stopPlaybackThread();
    }

    void setTapeParams(float drive, float age) {
        m_tapeDrive = drive;
        m_tapeAge = age;
        m_wowFlutter.setIntensity(0.001f * age, 0.002f * age);
    }

    bool load(const std::string& filePath) {
        std::cout << "[TrackNode] Loading file: " << filePath << std::endl;
        // Stop existing playback thread if any
        stopPlaybackThread();
        
        m_streamer = std::make_unique<DiskStreamer>();
        if (m_streamer->open(filePath)) {
            std::cout << "[TrackNode] File opened. Starting playback thread." << std::endl;
            startPlaybackThread();
            return true;
        }
        std::cerr << "[TrackNode] Failed to open file." << std::endl;
        return false;
    }

    bool startRecording(const std::string& filePath, int sampleRate, int channels) {
        m_recordingPath = filePath;
        drwav_data_format format;
        format.container = drwav_container_riff;
        format.format = DR_WAVE_FORMAT_PCM;
        format.channels = (drwav_uint32)channels;
        format.sampleRate = (drwav_uint32)sampleRate;
        format.bitsPerSample = 16;

        if (drwav_init_file_write(&m_wavWriter, filePath.c_str(), &format, NULL)) {
            m_isWriterOpen = true;
            m_state = TrackState::Recording;
            
            m_stopDiskThread = false;
            m_recordThread = std::make_unique<std::thread>([this, channels]() {
                std::vector<float> readBuf(1024 * channels);
                std::vector<int16_t> pcm(1024 * channels);
                
                while (!m_stopDiskThread || m_recordBuffer->getAvailableRead() > 0) {
                    size_t read = m_recordBuffer->read(readBuf.data(), readBuf.size());
                    if (read > 0) {
                        for (size_t i = 0; i < read; ++i) {
                            float s = readBuf[i];
                            if (s > 1.0f) s = 1.0f;
                            if (s < -1.0f) s = -1.0f;
                            pcm[i] = (int16_t)(s * 32767.0f);
                        }
                        drwav_write_pcm_frames(&m_wavWriter, (drwav_uint64)(read / channels), pcm.data());
                    } else {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                }
            });
            
            return true;
        }
        return false;
    }

    void stopRecording() {
        if (m_state != TrackState::Recording) return;
        
        m_state = TrackState::Idle;
        m_stopDiskThread = true;
        if (m_recordThread && m_recordThread->joinable()) {
            m_recordThread->join();
        }
        m_recordThread.reset();

        if (m_isWriterOpen) {
            drwav_uninit(&m_wavWriter);
            m_isWriterOpen = false;
        }

        // Auto-load the recording
        if (!m_recordingPath.empty()) {
            load(m_recordingPath);
        }
    }

    void process(float* buffer, int frames, int channels, size_t startFrame = 0) override {
        // 1. Capture/Read raw signal
        if (m_state == TrackState::Playing) {
            // Handle Seek Request from Audio Engine
            if (startFrame != (size_t)-1 && startFrame != m_lastProcessedFrame) {
                m_playbackBuffer->reset(); // Flush buffer on seek (Consumer side)
                seek(startFrame);
                m_lastProcessedFrame = startFrame; 
            }

            size_t read = m_playbackBuffer->read(buffer, frames * channels);
            size_t framesRead = read / channels;
            
            if (framesRead < (size_t)frames) {
                std::fill(buffer + framesRead * channels, buffer + frames * channels, 0.0f);
            }
            
            // Apply 5ms fade-in if we just seeked or started? 
            // Simplified: Fade in if we were starving (read == 0) and now have data?
            // For now, no fade-in to keep it simple, just avoid glitches.
            
            m_lastProcessedFrame += frames;
        }

        // 2. Apply Tape Physics
        for (int i = 0; i < frames; ++i) {
            m_wowFlutter.next(); 
            
            for (int c = 0; c < channels; ++c) {
                float& s = buffer[i * channels + c];
                s = AnalogBase::saturateLangevin(s, 1.0f + m_tapeDrive);
                
                if (m_tapeAge > 0.01f) {
                    m_ageFilters[c].setCutoff(20000.0f - (m_tapeAge * 15000.0f), 44100.0f);
                    s = m_ageFilters[c].process(s);
                }
            }
        }
    }

    void processRecording(const float* input, float* output, int frames, int channels) {
        // Copy input to output for monitoring
        std::copy(input, input + frames * channels, output);
        
        // Apply Tape Physics to monitoring output
        process(output, frames, channels, (size_t)-1);

        // Push PROCESSED signal to ring buffer for recording
        if (m_isWriterOpen) {
            m_recordBuffer->write(output, frames * channels);
        }
    }

    void setState(TrackState state) { 
        m_state = state; 
        if (state == TrackState::Idle) m_lastProcessedFrame = (size_t)-1;
    }
    TrackState getState() const { return m_state; }
    
    void seek(size_t frame) {
        m_seekTarget.store((int64_t)frame);
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
    void setName(const std::string& name) { m_name = name; }

private:
    void startPlaybackThread() {
        m_stopPlaybackThread = false;
        m_playbackThread = std::make_unique<std::thread>([this]() {
            std::cout << "[PlaybackThread] Thread started." << std::endl;
            std::vector<float> tempBuf(4096); // Read 2048 frames (stereo) at a time
            int channels = 2;

            while (!m_stopPlaybackThread) {
                // 1. Handle Seek
                int64_t target = m_seekTarget.exchange(-1);
                if (target != -1) {
                    m_streamer->seek((size_t)target);
                }

                // 2. Fill Buffer
                size_t space = m_playbackBuffer->getAvailableWrite();
                if (space >= tempBuf.size()) {
                    // Read from disk
                    size_t framesToRead = tempBuf.size() / channels;
                    size_t readFrames = m_streamer->read(tempBuf.data(), framesToRead, channels);
                    
                    if (readFrames > 0) {
                        m_playbackBuffer->write(tempBuf.data(), readFrames * channels);
                    } else {
                        // EOF or Error, sleep a bit
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                } else {
                    // Buffer full, sleep
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                }
            }
        });
    }

    void stopPlaybackThread() {
        m_stopPlaybackThread = true;
        if (m_playbackThread && m_playbackThread->joinable()) {
            m_playbackThread->join();
        }
        m_playbackThread.reset();
    }

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
    bool m_isWriterOpen = false;

    std::unique_ptr<std::thread> m_recordThread;
    std::atomic<bool> m_stopDiskThread{false};
    std::unique_ptr<LockFreeBuffer<float>> m_recordBuffer;

    // Playback Async
    std::unique_ptr<std::thread> m_playbackThread;
    std::atomic<bool> m_stopPlaybackThread{false};
    std::unique_ptr<LockFreeBuffer<float>> m_playbackBuffer;
    std::atomic<int64_t> m_seekTarget{-1};
    std::string m_recordingPath;
};

} // namespace Beam

#endif // TRACK_NODE_HPP