#include "disk_streamer.hpp"
#include "wav_reader.hpp"
#include <iostream>

namespace Beam {

DiskStreamer::DiskStreamer(size_t bufferSize) 
    : m_bufferSize(bufferSize), m_reader(std::make_unique<WavReader>()) {}

DiskStreamer::~DiskStreamer() {
    close();
}

bool DiskStreamer::open(const std::string& filePath) {
    m_filePath = filePath;
    return m_reader->open(filePath);
}

void DiskStreamer::close() {
    m_keepStreaming = false;
}

size_t DiskStreamer::read(float* output, size_t frames, int channels) {
    if (!m_reader) return 0;
    return m_reader->readFrames(output, frames, channels);
}

void DiskStreamer::seek(size_t frame) {
    if (m_reader) m_reader->seek(frame);
}

std::vector<float> DiskStreamer::getPeakData(int numPoints) {
    if (m_reader) return m_reader->getPeakData(numPoints);
    return {};
}

void DiskStreamer::streamLoop() {
    // Background streaming logic would go here if we were using a circular buffer
}

} // namespace Beam

