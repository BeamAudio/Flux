#ifndef WAV_READER_HPP
#define WAV_READER_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <mutex>

namespace Beam {

/**
 * @class WavReader
 * @brief Thread-safe reader for WAVE files with peak extraction support.
 */
class WavReader {
public:
    WavReader() : m_sampleRate(0), m_channels(0), m_bitsPerSample(0), m_dataSize(0), m_dataOffset(0), m_formatTag(0) {}

    bool open(const std::string& filePath) {
        std::lock_guard<std::recursive_mutex> lock(m_fileMutex);
        if (m_file.is_open()) m_file.close();
        
        m_file.open(filePath, std::ios::binary);
        if (!m_file.is_open()) return false;

        char buffer[4];
        if (!m_file.read(buffer, 4) || std::string(buffer, 4) != "RIFF") return false;
        m_file.seekg(8, std::ios::beg); 
        if (!m_file.read(buffer, 4) || std::string(buffer, 4) != "WAVE") return false;

        bool foundFmt = false;
        while (m_file.read(buffer, 4)) {
            uint32_t chunkSize;
            if (!m_file.read(reinterpret_cast<char*>(&chunkSize), 4)) break;
            if (std::string(buffer, 4) == "fmt ") {
                m_file.read(reinterpret_cast<char*>(&m_formatTag), 2);
                m_file.read(reinterpret_cast<char*>(&m_channels), 2);
                m_file.read(reinterpret_cast<char*>(&m_sampleRate), 4);
                m_file.seekg(6, std::ios::cur); 
                m_file.read(reinterpret_cast<char*>(&m_bitsPerSample), 2);
                if (chunkSize > 16) m_file.seekg(chunkSize - 16, std::ios::cur);
                if (m_channels > 0 && m_bitsPerSample > 0) foundFmt = true;
                break;
            } else { m_file.seekg(chunkSize, std::ios::cur); }
        }

        if (!foundFmt) return false;

        bool foundData = false;
        while (m_file.read(buffer, 4)) {
            uint32_t chunkSize;
            if (!m_file.read(reinterpret_cast<char*>(&chunkSize), 4)) break;
            if (std::string(buffer, 4) == "data") {
                m_dataSize = chunkSize;
                m_dataOffset = (uint32_t)m_file.tellg();
                foundData = true;
                break;
            } else { m_file.seekg(chunkSize, std::ios::cur); }
        }
        return foundData;
    }

    size_t readFrames(float* buffer, size_t frames, int destChannels) {
        std::lock_guard<std::recursive_mutex> lock(m_fileMutex);
        if (!m_file.is_open() || m_channels == 0 || m_bitsPerSample == 0) return 0;

        size_t samplesToRead = frames * m_channels;
        std::vector<float> srcBuffer(samplesToRead, 0.0f);
        size_t totalBytesRead = 0;

        if (m_bitsPerSample == 16) {
            std::vector<int16_t> intBuffer(samplesToRead);
            m_file.read(reinterpret_cast<char*>(intBuffer.data()), samplesToRead * 2);
            totalBytesRead = (size_t)m_file.gcount();
            size_t samplesRead = totalBytesRead / 2;
            for (size_t i = 0; i < samplesRead; ++i) srcBuffer[i] = intBuffer[i] / 32768.0f;
        } else if (m_bitsPerSample == 32) {
            m_file.read(reinterpret_cast<char*>(srcBuffer.data()), samplesToRead * 4);
            totalBytesRead = (size_t)m_file.gcount();
        }

        size_t framesRead = totalBytesRead / (m_channels * (m_bitsPerSample / 8));
        for (size_t f = 0; f < framesRead && f < frames; ++f) {
            for (int dc = 0; dc < destChannels; ++dc) {
                float sample = (dc < m_channels) ? srcBuffer[f * m_channels + dc] : srcBuffer[f * m_channels];
                buffer[f * destChannels + dc] += sample;
            }
        }
        return framesRead;
    }

    void seek(size_t frame) {
        std::lock_guard<std::recursive_mutex> lock(m_fileMutex);
        if (!m_file.is_open()) return;
        uint32_t bytesPerFrame = m_channels * (m_bitsPerSample / 8);
        m_file.clear();
        m_file.seekg(m_dataOffset + (frame * bytesPerFrame), std::ios::beg);
    }

    std::vector<float> getPeakData(int numPoints) {
        std::lock_guard<std::recursive_mutex> lock(m_fileMutex);
        if (!m_file.is_open() || numPoints <= 0 || m_channels == 0) return {};
        
        size_t bytesPerFrame = m_channels * (m_bitsPerSample / 8);
        size_t totalFrames = m_dataSize / bytesPerFrame;
        if (totalFrames == 0) return std::vector<float>(numPoints, 0.0f);

        size_t framesPerPoint = totalFrames / numPoints;
        if (framesPerPoint == 0) framesPerPoint = 1;
        
        std::vector<float> peaks;
        peaks.reserve(numPoints);

        seek(0);
        const size_t CHUNK_SIZE = 16384; 
        std::vector<float> chunk(CHUNK_SIZE * m_channels);
        
        for (int i = 0; i < numPoints; ++i) {
            float maxPeak = 0.0f;
            size_t remaining = framesPerPoint;
            while(remaining > 0) {
                size_t toRead = (std::min)(remaining, CHUNK_SIZE);
                std::fill(chunk.begin(), chunk.end(), 0.0f);
                size_t read = readFrames(chunk.data(), toRead, m_channels);
                if (read == 0) break;
                for (size_t s = 0; s < read * m_channels; ++s) {
                    float v = std::abs(chunk[s]);
                    if (v > maxPeak) maxPeak = v;
                }
                remaining -= read;
            }
            peaks.push_back(maxPeak);
        }
        seek(0);
        return peaks;
    }

    uint32_t getSampleRate() const { return m_sampleRate; }
    uint16_t getChannels() const { return m_channels; }

private:
    std::ifstream m_file;
    std::recursive_mutex m_fileMutex;
    uint32_t m_sampleRate;
    uint16_t m_channels;
    uint16_t m_bitsPerSample;
    uint16_t m_formatTag;
    uint32_t m_dataSize;
    uint32_t m_dataOffset;
};

} // namespace Beam

#endif // WAV_READER_HPP
