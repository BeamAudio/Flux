#include "disk_streamer.hpp"
#include "wav_reader.hpp"
#include <iostream>

namespace Beam {

DiskStreamer::DiskStreamer(size_t bufferSize) 
    : m_bufferSize(bufferSize), m_keepStreaming(false) {
}

DiskStreamer::~DiskStreamer() {
    close();
}

bool DiskStreamer::open(const std::string& filePath) {
    m_reader = std::make_unique<WavReader>();
    if (!m_reader->open(filePath)) {
        return false;
    }
    
    m_keepStreaming = true;
    return true;
}

void DiskStreamer::close() {
    m_keepStreaming = false;
    m_reader.reset();
}

size_t DiskStreamer::read(float* output, size_t frames) {
    if (!m_reader) return 0;
    return m_reader->readFrames(output, frames);
}

void DiskStreamer::streamLoop() {
    // Background filling can be re-implemented later
}

} // namespace Beam