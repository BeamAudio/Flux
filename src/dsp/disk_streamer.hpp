#ifndef DISK_STREAMER_HPP
#define DISK_STREAMER_HPP

#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace Beam {

class WavReader; // Forward decl

class DiskStreamer {
public:
    DiskStreamer(size_t bufferSize = 4096 * 4);
    ~DiskStreamer();

    bool open(const std::string& filePath);
    void close();

    // To be called by Audio Thread
    size_t read(float* output, size_t frames, int channels);
    void seek(size_t frame);

private:
    void streamLoop();

    std::string m_filePath;
    std::atomic<bool> m_keepStreaming{false};
    std::unique_ptr<WavReader> m_reader;
    size_t m_bufferSize;
};

} // namespace Beam

#endif // DISK_STREAMER_HPP
