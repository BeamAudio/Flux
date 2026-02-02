#include <iostream>
#include <fstream>
#include <vector>
#include <string>

namespace Beam {

class WavWriter {
public:
    static bool write(const std::string& filename, const float* buffer, size_t numSamples, int sampleRate, int channels) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;

        // RIFF header
        file.write("RIFF", 4);
        int32_t fileSize = 36 + numSamples * sizeof(int16_t);
        file.write(reinterpret_cast<char*>(&fileSize), 4);
        file.write("WAVE", 4);

        // fmt chunk
        file.write("fmt ", 4);
        int32_t fmtSize = 16;
        file.write(reinterpret_cast<char*>(&fmtSize), 4);
        int16_t format = 1; // PCM
        file.write(reinterpret_cast<char*>(&format), 2);
        int16_t numChannels = static_cast<int16_t>(channels);
        file.write(reinterpret_cast<char*>(&numChannels), 2);
        int32_t sRate = sampleRate;
        file.write(reinterpret_cast<char*>(&sRate), 4);
        int32_t byteRate = sampleRate * channels * sizeof(int16_t);
        file.write(reinterpret_cast<char*>(&byteRate), 4);
        int16_t blockAlign = channels * sizeof(int16_t);
        file.write(reinterpret_cast<char*>(&blockAlign), 2);
        int16_t bitsPerSample = 16;
        file.write(reinterpret_cast<char*>(&bitsPerSample), 2);

        // data chunk
        file.write("data", 4);
        int32_t dataSize = numSamples * sizeof(int16_t);
        file.write(reinterpret_cast<char*>(&dataSize), 4);

        for (size_t i = 0; i < numSamples; ++i) {
            float sample = buffer[i];
            if (sample > 1.0f) sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;
            int16_t intSample = static_cast<int16_t>(sample * 32767.0f);
            file.write(reinterpret_cast<char*>(&intSample), 2);
        }

        return true;
    }
};

} // namespace Beam






