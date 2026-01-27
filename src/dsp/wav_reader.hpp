#ifndef WAV_READER_HPP
#define WAV_READER_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>

namespace Beam {

class WavReader {
public:
    WavReader() : m_sampleRate(0), m_channels(0), m_bitsPerSample(0) {}

    bool open(const std::string& filePath) {
        m_file.open(filePath, std::ios::binary);
        if (!m_file.is_open()) return false;

        char buffer[4];
        m_file.read(buffer, 4);
        if (std::string(buffer, 4) != "RIFF") return false;

        m_file.seekg(8, std::ios::beg); 
        m_file.read(buffer, 4);
        if (std::string(buffer, 4) != "WAVE") return false;

        bool foundFmt = false;
        while (m_file.read(buffer, 4)) {
            uint32_t chunkSize;
            m_file.read(reinterpret_cast<char*>(&chunkSize), 4);
            if (std::string(buffer, 4) == "fmt ") {
                m_file.read(reinterpret_cast<char*>(&m_formatTag), 2);
                m_file.read(reinterpret_cast<char*>(&m_channels), 2);
                m_file.read(reinterpret_cast<char*>(&m_sampleRate), 4);
                m_file.seekg(6, std::ios::cur); 
                m_file.read(reinterpret_cast<char*>(&m_bitsPerSample), 2);
                if (chunkSize > 16) m_file.seekg(chunkSize - 16, std::ios::cur);
                
                if (m_channels == 0) return false;
                
                foundFmt = true;
                break;
            } else { m_file.seekg(chunkSize, std::ios::cur); }
        }

        if (!foundFmt) return false;

        bool foundData = false;
        while (m_file.read(buffer, 4)) {
            uint32_t chunkSize;
            m_file.read(reinterpret_cast<char*>(&chunkSize), 4);
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
        if (!m_file.is_open()) return 0;

        std::vector<float> srcBuffer(frames * m_channels);
        size_t framesRead = 0;

        if (m_bitsPerSample == 16) {
            std::vector<int16_t> intBuffer(frames * m_channels);
            m_file.read(reinterpret_cast<char*>(intBuffer.data()), intBuffer.size() * 2);
            framesRead = m_file.gcount() / (m_channels * 2);
            for (size_t i = 0; i < framesRead * m_channels; ++i) {
                srcBuffer[i] = intBuffer[i] / 32768.0f;
            }
        } else if (m_bitsPerSample == 32) {
            m_file.read(reinterpret_cast<char*>(srcBuffer.data()), srcBuffer.size() * 4);
            framesRead = m_file.gcount() / (m_channels * 4);
        }

        for (size_t f = 0; f < framesRead && f < frames; ++f) {
            for (int dc = 0; dc < destChannels; ++dc) {
                float sample = 0.0f;
                if (dc < m_channels) {
                    sample = srcBuffer[f * m_channels + dc];
                } else {
                    sample = srcBuffer[f * m_channels + 0];
                }
                buffer[f * destChannels + dc] += sample;
            }
        }

        return framesRead;
    }

    void seek(size_t frame) {
        if (!m_file.is_open()) return;
        uint32_t bytesPerFrame = m_channels * (m_bitsPerSample / 8);
        m_file.clear();
        m_file.seekg(m_dataOffset + (frame * bytesPerFrame), std::ios::beg);
    }

    uint32_t getSampleRate() const { return m_sampleRate; }
    uint16_t getChannels() const { return m_channels; }

private:
    std::ifstream m_file;
    uint32_t m_sampleRate;
    uint16_t m_channels;
    uint16_t m_bitsPerSample;
    uint16_t m_formatTag;
    uint32_t m_dataSize;
    uint32_t m_dataOffset;
};

} // namespace Beam

#endif // WAV_READER_HPP
