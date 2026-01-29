#ifndef AUDIO_READER_HPP
#define AUDIO_READER_HPP

#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include "../../third_party/miniaudio.h"

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

    std::vector<std::vector<float>> getPeakData(int numPoints) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_isInitialized || numPoints <= 0) return {};
        
        uint64_t totalFrames = getTotalFrames();
        int channels = (int)m_decoder.outputChannels;
        if (totalFrames == 0) return std::vector<std::vector<float>>(channels, std::vector<float>(numPoints, 0.0f));

        uint64_t framesPerPoint = totalFrames / numPoints;
        if (framesPerPoint == 0) framesPerPoint = 1;
        
        std::vector<std::vector<float>> allPeaks(channels, std::vector<float>(numPoints, 0.0f));

        ma_uint64 originalPos = 0;
        ma_decoder_get_cursor_in_pcm_frames(&m_decoder, &originalPos);
        ma_decoder_seek_to_pcm_frame(&m_decoder, 0);

        const size_t CHUNK_SIZE = 4096;
        std::vector<float> chunk(CHUNK_SIZE * channels);
        
        for (int i = 0; i < numPoints; ++i) {
            uint64_t remaining = framesPerPoint;
            while(remaining > 0) {
                size_t toRead = (size_t)(std::min)((uint64_t)CHUNK_SIZE, remaining);
                ma_uint64 read = 0;
                ma_decoder_read_pcm_frames(&m_decoder, chunk.data(), toRead, &read);
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
        
        ma_decoder_seek_to_pcm_frame(&m_decoder, originalPos);
        return allPeaks;
    }

private:
    ma_decoder m_decoder;
    bool m_isInitialized;
    std::mutex m_mutex;
};

} // namespace Beam

#endif // AUDIO_READER_HPP

