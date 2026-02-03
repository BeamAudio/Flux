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

    void registerPlugin(const std::string& name, const std::string& category, FactoryFunc factory) {
        m_factories[name] = factory;
        m_categories[name] = category;
        m_pluginNames.push_back(name);
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

    std::function<void()> onRegistryChanged;

private:
    PluginRegistry() {}
    std::map<std::string, FactoryFunc> m_factories;
    std::map<std::string, std::string> m_categories;
    std::vector<std::string> m_pluginNames;
};

// Helper macro for registration
#define REGISTER_PLUGIN(ClassType, Name) \
    struct ClassType##Registrar { \
        ClassType##Registrar() { \
            PluginRegistry::get().registerPlugin(Name, [](int buf, float sr) { \
                return std::make_shared<ClassType>(buf, sr); \
            }); \
        } \
    }; \
    static ClassType##Registrar global_##ClassType##Registrar;

} // namespace Beam

#endif // PLUGIN_REGISTRY_HPP
