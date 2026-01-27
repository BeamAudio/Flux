#ifndef WAV_READER_HPP
#define WAV_READER_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>

namespace Beam {

struct WavHeader {
    char riff[4];
    uint32_t fileSize;
    char wave[4];
    char fmt[4];
    uint32_t fmtSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char data[4];
    uint32_t dataSize;
};

class WavReader {
public:
    WavReader() : m_sampleRate(0), m_channels(0), m_totalFrames(0) {}

    bool open(const std::string& filePath) {
        m_file.open(filePath, std::ios::binary);
        if (!m_file.is_open()) return false;

        WavHeader header;
        m_file.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));

        // Basic verification
        if (std::string(header.riff, 4) != "RIFF" || std::string(header.wave, 4) != "WAVE") {
            return false;
        }

        m_sampleRate = header.sampleRate;
        m_channels = header.numChannels;
        m_totalFrames = header.dataSize / (m_channels * (header.bitsPerSample / 8));
        m_dataOffset = sizeof(WavHeader);
        
        return true;
    }

    size_t readFrames(float* buffer, size_t frames) {
        if (!m_file.is_open()) return 0;

        std::vector<int16_t> intBuffer(frames * m_channels);
        m_file.read(reinterpret_cast<char*>(intBuffer.data()), intBuffer.size() * sizeof(int16_t));
        size_t bytesRead = m_file.gcount();
        size_t framesRead = bytesRead / (m_channels * sizeof(int16_t));

        for (size_t i = 0; i < framesRead * m_channels; ++i) {
            buffer[i] = intBuffer[i] / 32768.0f;
        }

        return framesRead;
    }

    uint32_t getSampleRate() const { return m_sampleRate; }
    uint16_t getChannels() const { return m_channels; }

private:
    std::ifstream m_file;
    uint32_t m_sampleRate;
    uint16_t m_channels;
    uint64_t m_totalFrames;
    size_t m_dataOffset;
};

} // namespace Beam

#endif // WAV_READER_HPP
