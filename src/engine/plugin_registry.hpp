#ifndef PLUGIN_REGISTRY_HPP
#define PLUGIN_REGISTRY_HPP

#include "flux_node.hpp"
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

    void registerPlugin(const std::string& name, FactoryFunc factory) {
        m_factories[name] = factory;
        m_pluginNames.push_back(name);
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

private:
    PluginRegistry() {}
    std::map<std::string, FactoryFunc> m_factories;
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
