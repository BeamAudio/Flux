#ifndef APP_SETTINGS_HPP
#define APP_SETTINGS_HPP

#include "json.hpp"
#include <fstream>

namespace Beam {

struct AudioSettings {
    double sampleRate = 44100.0;
    int bufferSize = 512;
    std::string outputDeviceId = "";
    std::string inputDeviceId = "";
};

struct AppSettings {
    bool autosaveEnabled = true;
    float autosaveIntervalMinutes = 5.0f;
    AudioSettings audio;
    
    // Serialization
    nlohmann::json serialize() const {
        return {
            {"autosaveEnabled", autosaveEnabled},
            {"autosaveInterval", autosaveIntervalMinutes},
            {"audio", {
                {"sampleRate", audio.sampleRate},
                {"bufferSize", audio.bufferSize},
                {"outputDeviceId", audio.outputDeviceId},
                {"inputDeviceId", audio.inputDeviceId}
            }}
        };
    }

    void deserialize(const nlohmann::json& j) {
        if (j.contains("autosaveEnabled")) autosaveEnabled = j["autosaveEnabled"];
        if (j.contains("autosaveInterval")) autosaveIntervalMinutes = j["autosaveInterval"];
        if (j.contains("audio")) {
            auto& a = j["audio"];
            if (a.contains("sampleRate")) audio.sampleRate = a["sampleRate"];
            if (a.contains("bufferSize")) audio.bufferSize = a["bufferSize"];
            if (a.contains("outputDeviceId")) audio.outputDeviceId = a["outputDeviceId"];
            if (a.contains("inputDeviceId")) audio.inputDeviceId = a["inputDeviceId"];
        }
    }

    void save(const std::string& path = "settings.json") {
        std::ofstream file(path);
        if (file.is_open()) {
            file << serialize().dump(4);
        }
    }

    void load(const std::string& path = "settings.json") {
        std::ifstream file(path);
        if (file.is_open()) {
            try {
                nlohmann::json j;
                file >> j;
                deserialize(j);
            } catch(...) {}
        }
    }
};

} // namespace Beam

#endif // APP_SETTINGS_HPP
