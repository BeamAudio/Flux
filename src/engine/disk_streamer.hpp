#ifndef DISK_STREAMER_HPP
#define DISK_STREAMER_HPP

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include "audio_reader.hpp"

namespace Beam {

/**
 * @class DiskStreamer
 * @brief Manages audio data retrieval from disk using AudioReader.
 */
class DiskStreamer {
public:
    DiskStreamer(size_t bufferSize = 44100 * 2);
    ~DiskStreamer();

    bool open(const std::string& filePath, int channels = 2);
    void close();

    size_t read(float* output, size_t frames, int channels);
    
    /**
     * @brief Seeks to an absolute frame position in the source file.
     */
    void seek(size_t frame);

    std::vector<std::vector<float>> getPeakData(int numPoints);

    uint64_t getTotalFrames() const { return m_reader ? m_reader->getTotalFrames() : 0; }

private:
    std::string m_filePath;
    std::unique_ptr<AudioReader> m_reader;
    size_t m_bufferSize;
};

} // namespace Beam

#endif // DISK_STREAMER_HPP






