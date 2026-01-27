#ifndef DISK_STREAMER_HPP
#define DISK_STREAMER_HPP

#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace Beam {

class DiskStreamer {
public:
    DiskStreamer(size_t bufferSize = 4096 * 4);
    ~DiskStreamer();

    bool open(const std::string& filePath);
    void close();

    // To be called by Audio Thread
    size_t read(float* output, size_t frames);

private:
    void streamLoop();

    std::string m_filePath;
    std::vector<float> m_bufferA;
    std::vector<float> m_bufferB;
    
    std::atomic<bool> m_useBufferA{true};
    std::atomic<bool> m_bufferAReady{false};
    std::atomic<bool> m_bufferBReady{false};
    
    std::atomic<bool> m_keepStreaming{false};
    std::thread m_streamThread;
    
    std::mutex m_streamMutex;
    std::condition_variable m_cv;
    
    size_t m_bufferSize;
    size_t m_readPointer{0};
    
    void* m_wavHandle{nullptr}; // Will be drwav*
};

} // namespace Beam

#endif // DISK_STREAMER_HPP
