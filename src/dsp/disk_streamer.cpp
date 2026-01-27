#include "disk_streamer.hpp"
#include <iostream>

// Note: dr_wav.h should be in third_party
// #define DR_WAV_IMPLEMENTATION
// #include "third_party/dr_wav.h"

namespace Beam {

DiskStreamer::DiskStreamer(size_t bufferSize) 
    : m_bufferSize(bufferSize), m_keepStreaming(false) {
    m_bufferA.resize(bufferSize);
    m_bufferB.resize(bufferSize);
}

DiskStreamer::~DiskStreamer() {
    close();
}

bool DiskStreamer::open(const std::string& filePath) {
    m_filePath = filePath;
    // Mocking drwav_open_file
    m_wavHandle = (void*)1; // Placeholder
    
    m_keepStreaming = true;
    m_streamThread = std::thread(&DiskStreamer::streamLoop, this);
    
    return true;
}

void DiskStreamer::close() {
    m_keepStreaming = false;
    m_cv.notify_all();
    if (m_streamThread.joinable()) {
        m_streamThread.join();
    }
    m_wavHandle = nullptr;
}

size_t DiskStreamer::read(float* output, size_t frames) {
    // simplified double-buffering logic
    // ...
    return frames;
}

void DiskStreamer::streamLoop() {
    while (m_keepStreaming) {
        std::unique_lock<std::mutex> lock(m_streamMutex);
        m_cv.wait(lock, [this] { return !m_keepStreaming || (!m_bufferAReady || !m_bufferBReady); });
        
        if (!m_keepStreaming) break;

        // Fill empty buffers using dr_wav
        if (!m_bufferAReady) {
            // drwav_read_pcm_frames_f32(m_wavHandle, m_bufferSize / channels, m_bufferA.data());
            m_bufferAReady = true;
        }
        if (!m_bufferBReady) {
            // drwav_read_pcm_frames_f32(m_wavHandle, m_bufferSize / channels, m_bufferB.data());
            m_bufferBReady = true;
        }
    }
}

} // namespace Beam
