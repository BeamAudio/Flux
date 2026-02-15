#ifndef PLUGIN_REGISTRY_HPP
#define PLUGIN_REGISTRY_HPP

#include "engine/core/flux_node.hpp"
#include <string>
#include <map>
#include <functional>
#include <memory>
#include <vector>

namespace Beam {

/**
 * @struct PluginMetadata
 * @brief Detailed information about a plugin
 */
struct PluginMetadata {
    std::string author = "Beam Audio";
    std::string description = "";
    std::string version = "1.0.0";
    std::string website = "";
    std::vector<std::string> tags;
};

/**
 * @class PluginRegistry
 * @brief Singleton registry for available FluxNodes (Plugins).
 * Allows dynamic registration and instantiation of DSP effects.
 */
class PluginRegistry {
public:
    using FactoryFunc = std::function<std::shared_ptr<FluxNode>(int, float)>;

    static PluginRegistry& get() {
        static PluginRegistry instance;
        return instance;
    }

    void registerPlugin(const std::string& name, const std::string& category, FactoryFunc factory, const PluginMetadata& metadata = {}) {
        m_factories[name] = factory;
        m_categories[name] = category;
        m_metadata[name] = metadata;
        
        bool alreadyExists = false;
        for (const auto& n : m_pluginNames) {
            if (n == name) { alreadyExists = true; break; }
        }
        if (!alreadyExists) {
            m_pluginNames.push_back(name);
        }
        
        if (onRegistryChanged) onRegistryChanged();
    }

    // Overload for backward compatibility (defaults to "Native")
    void registerPlugin(const std::string& name, FactoryFunc factory) {
        registerPlugin(name, "Native", factory);
    }

    std::shared_ptr<FluxNode> createPlugin(const std::string& name, int bufferSize, float sampleRate) {
        if (m_factories.find(name) != m_factories.end()) {
            return m_factories[name](bufferSize, sampleRate);
        }
        return nullptr;
    }

    const std::vector<std::string>& getAvailablePlugins() const {
        return m_pluginNames;
    }

    std::string getPluginCategory(const std::string& name) const {
        if (m_categories.count(name)) return m_categories.at(name);
        return "Native";
    }

    const PluginMetadata& getPluginMetadata(const std::string& name) const {
        static PluginMetadata empty;
        if (m_metadata.count(name)) return m_metadata.at(name);
        return empty;
    }

    std::function<void()> onRegistryChanged;

private:
    PluginRegistry() {}
    std::map<std::string, FactoryFunc> m_factories;
    std::map<std::string, std::string> m_categories;
    std::map<std::string, PluginMetadata> m_metadata;
    std::vector<std::string> m_pluginNames;
};

// Helper for unique names
#define BEAM_CAT_IMPL(a, b) a##b
#define BEAM_CAT(a, b) BEAM_CAT_IMPL(a, b)
#define BEAM_UNIQUE_NAME(prefix) BEAM_CAT(prefix, __LINE__)

// Standard registration for all Beam plugins (internal and external)
#define REGISTER_BEAM_PLUGIN(ClassType) \
    struct BEAM_UNIQUE_NAME(PluginRegistrar) { \
        BEAM_UNIQUE_NAME(PluginRegistrar)() { \
            auto p = std::make_shared<ClassType>(0, 44100.0f); \
            PluginRegistry::get().registerPlugin(p->getName(), p->getCategory(), [](int b, float s) { \
                return std::make_shared<ClassType>(b, s); \
            }, p->getMetadata()); \
        } \
    }; \
    static BEAM_UNIQUE_NAME(PluginRegistrar) BEAM_UNIQUE_NAME(global_registrar);

} // namespace Beam

#endif // PLUGIN_REGISTRY_HPP
