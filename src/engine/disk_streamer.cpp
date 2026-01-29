#include "disk_streamer.hpp"
#include <iostream>

namespace Beam {

DiskStreamer::DiskStreamer(size_t bufferSize) 
    : m_bufferSize(bufferSize), m_reader(std::make_unique<AudioReader>()) {}

DiskStreamer::~DiskStreamer() {
    close();
}

bool DiskStreamer::open(const std::string& filePath, int channels) {
    m_filePath = filePath;
    return m_reader->open(filePath, channels);
}

void DiskStreamer::close() {
    if (m_reader) m_reader->close();
}

size_t DiskStreamer::read(float* output, size_t frames, int channels) {
    if (!m_reader) return 0;
    return m_reader->readFrames(output, frames, channels);
}

void DiskStreamer::seek(size_t frame) {
    if (m_reader) m_reader->seek(frame);
}

std::vector<std::vector<float>> DiskStreamer::getPeakData(int numPoints) {
    if (m_reader) return m_reader->getPeakData(numPoints);
    return {};
}

} // namespace Beam

