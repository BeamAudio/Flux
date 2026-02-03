#ifndef PLUGIN_LIBRARY_HPP
#define PLUGIN_LIBRARY_HPP

#include <string>
#include <vector>
#include <set>
#include "json.hpp"
#include <fstream>
#include <filesystem>

namespace Beam {

struct PluginEntry {
    std::string name;
    std::string path;
    std::string category;
    std::string type; // "VST3", "FluxScript", "Native"

    nlohmann::json serialize() const {
        return { {"name", name}, {"path", path}, {"category", category}, {"type", type} };
    }

    static PluginEntry deserialize(const nlohmann::json& j) {
        return { j["name"], j["path"], j["category"], j["type"] };
    }
};

/**
 * @class PluginLibrary
 * @brief Manages the persistent list of known plugins and scan paths.
 */
class PluginLibrary {
public:
    static PluginLibrary& get() {
        static PluginLibrary instance;
        return instance;
    }

    void addEntry(const PluginEntry& entry) {
        std::cout << "[PluginLibrary] Adding entry: " << entry.name << " Cat: " << entry.category << " Type: " << entry.type << std::endl;
        std::cout.flush();
        for (auto& existing : m_entries) {
            if (existing.path == entry.path) {
                existing.name = entry.name;
                existing.category = entry.category;
                existing.type = entry.type;
                save();
                return;
            }
        }
        m_entries.push_back(entry);
        save();
    }

    const std::vector<PluginEntry>& getEntries() const { return m_entries; }

    void addScanPath(const std::string& path) {
        if (m_scanPaths.find(path) == m_scanPaths.end()) {
            m_scanPaths.insert(path);
            save();
        }
    }

    const std::set<std::string>& getScanPaths() const { return m_scanPaths; }

    void load() {
        std::ifstream f("plugin_library.json");
        if (!f.is_open()) {
            std::cout << "[PluginLibrary] Creating new plugin_library.json" << std::endl;
            // Defaults
            #ifdef _WIN32
            addScanPath("C:/Program Files/Common Files/VST3");
            #endif
            addScanPath("plugins");
            return;
        }

        try {
            nlohmann::json data;
            f >> data;
            if (data.contains("entries")) {
                m_entries.clear();
                for (auto& j : data["entries"]) m_entries.push_back(PluginEntry::deserialize(j));
            }
            if (data.contains("scanPaths")) {
                m_scanPaths.clear();
                for (auto& p : data["scanPaths"]) m_scanPaths.insert(p);
            }
            std::cout << "[PluginLibrary] Loaded " << m_entries.size() << " entries from disk." << std::endl;
            std::cout.flush();
        } catch(...) {
            std::cerr << "[PluginLibrary] Failed to parse JSON!" << std::endl;
        }
    }

    void save() {
        nlohmann::json data;
        nlohmann::json entries = nlohmann::json::array();
        for (auto& e : m_entries) entries.push_back(e.serialize());
        data["entries"] = entries;
        
        nlohmann::json paths = nlohmann::json::array();
        for (auto& p : m_scanPaths) paths.push_back(p);
        data["scanPaths"] = paths;

        std::ofstream f("plugin_library.json");
        if (f.is_open()) {
            f << data.dump(4);
            std::cout << "[PluginLibrary] Saved " << m_entries.size() << " entries to disk." << std::endl;
            std::cout.flush();
        }
    }

private:
    PluginLibrary() { load(); }
    std::vector<PluginEntry> m_entries;
    std::set<std::string> m_scanPaths;
};

} // namespace Beam

#endif
