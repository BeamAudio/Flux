#ifndef AUDIO_READER_HPP
#define AUDIO_READER_HPP

#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include <thread>
#include <functional>
#include "miniaudio.h"

namespace Beam {

/**
 * @class AudioReader
 * @brief Universal audio reader using miniaudio's decoder.
 * Supports WAV, MP3, FLAC, etc.
 */
class AudioReader {
public:
    AudioReader() : m_isInitialized(false) {}
    ~AudioReader() { close(); }

    bool open(const std::string& filePath, int targetChannels = 2) {
        std::lock_guard<std::mutex> lock(m_mutex);
        close();
        
        m_filePath = filePath; // Store path for async peak generation

        ma_decoder_config config = ma_decoder_config_init(ma_format_f32, (ma_uint32)targetChannels, 0); 
        ma_result result = ma_decoder_init_file(filePath.c_str(), &config, &m_decoder);
        
        if (result != MA_SUCCESS) return false;

        m_isInitialized = true;
        return true;
    }

    void close() {
        if (m_isInitialized) {
            ma_decoder_uninit(&m_decoder);
            m_isInitialized = false;
        }
    }

    size_t readFrames(float* buffer, size_t frames, int destChannels) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_isInitialized) return 0;

        ma_uint64 framesRead = 0;
        ma_decoder_read_pcm_frames(&m_decoder, buffer, frames, &framesRead);
        return (size_t)framesRead;
    }

    void seek(size_t frame) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_isInitialized) {
            ma_decoder_seek_to_pcm_frame(&m_decoder, frame);
        }
    }

    uint32_t getSampleRate() const { return m_isInitialized ? m_decoder.outputSampleRate : 0; }
    uint32_t getChannels() const { return m_isInitialized ? m_decoder.outputChannels : 0; }
    
    uint64_t getTotalFrames() const {
        if (!m_isInitialized) return 0;
        ma_uint64 length = 0;
        ma_decoder_get_length_in_pcm_frames(const_cast<ma_decoder*>(&m_decoder), &length);
        return length;
    }

    // Synchronous (Blocking) - Legacy
    std::vector<std::vector<float>> getPeakData(int numPoints) {
        // ... (Keep existing implementation logic if needed, or implement via temporary decoder too?)
        // Let's reimplement using temporary decoder to be safe even in sync mode
        if (m_filePath.empty() || numPoints <= 0) return {};
        
        ma_decoder tempDecoder;
        ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0); // Native channels
        if (ma_decoder_init_file(m_filePath.c_str(), &config, &tempDecoder) != MA_SUCCESS) return {};
        
        auto peaks = computePeaksInternal(&tempDecoder, numPoints);
        ma_decoder_uninit(&tempDecoder);
        return peaks;
    }
    
    using PeakCallback = std::function<void(const std::vector<std::vector<float>>&)>;

    // Asynchronous (Non-blocking)
    void getPeakDataAsync(int numPoints, PeakCallback callback) {
        if (m_filePath.empty() || numPoints <= 0) return;
        
        std::string pathCopy = m_filePath; // Capture by value
        
        std::thread([pathCopy, numPoints, callback]() {
            ma_decoder tempDecoder;
            ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0); 
            if (ma_decoder_init_file(pathCopy.c_str(), &config, &tempDecoder) != MA_SUCCESS) {
                // Callback with empty data on failure
                if (callback) callback({});
                return;
            }
            
            auto peaks = computePeaksInternal(&tempDecoder, numPoints);
            ma_decoder_uninit(&tempDecoder);
            
            if (callback) callback(peaks);
        }).detach();
    }

private:
    static std::vector<std::vector<float>> computePeaksInternal(ma_decoder* decoder, int numPoints) {
        ma_uint64 totalFrames = 0;
        ma_decoder_get_length_in_pcm_frames(decoder, &totalFrames);
        int channels = (int)decoder->outputChannels;
        
        if (totalFrames == 0) return std::vector<std::vector<float>>(channels, std::vector<float>(numPoints, 0.0f));

        uint64_t framesPerPoint = totalFrames / numPoints;
        if (framesPerPoint == 0) framesPerPoint = 1;
        
        std::vector<std::vector<float>> allPeaks(channels, std::vector<float>(numPoints, 0.0f));

        const size_t CHUNK_SIZE = 4096;
        std::vector<float> chunk(CHUNK_SIZE * channels);
        
        for (int i = 0; i < numPoints; ++i) {
            uint64_t remaining = framesPerPoint;
            while(remaining > 0) {
                size_t toRead = (size_t)(std::min)((uint64_t)CHUNK_SIZE, remaining);
                ma_uint64 read = 0;
                ma_decoder_read_pcm_frames(decoder, chunk.data(), toRead, &read);
                if (read == 0) break;
                
                for (ma_uint64 f = 0; f < read; ++f) {
                    for (int c = 0; c < channels; ++c) {
                        float v = std::abs(chunk[f * channels + c]);
                        if (v > allPeaks[c][i]) allPeaks[c][i] = v;
                    }
                }
                remaining -= read;
            }
        }
        return allPeaks;
    }

    ma_decoder m_decoder;
    std::string m_filePath;
    bool m_isInitialized;
    std::mutex m_mutex;
};

} // namespace Beam

#endif // AUDIO_READER_HPP




