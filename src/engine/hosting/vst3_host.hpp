#ifndef VST3_HOST_HPP
#define VST3_HOST_HPP

#include "engine/core/flux_node.hpp"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include <string>
#include <memory>

namespace Beam {

/**
 * @class VST3HostNode
 * @brief A FluxNode that wraps a third-party VST3 plugin.
 */
class VST3HostNode : public FluxNode, public Steinberg::Vst::IHostApplication, public Steinberg::Vst::IComponentHandler {
public:
    VST3HostNode(const std::string& pluginPath);
    ~VST3HostNode() override;

    bool load();

    std::string getName() const override;
    std::vector<Port> getInputPorts() const override;
    std::vector<Port> getOutputPorts() const override;
    size_t getLatency() const override;

    std::shared_ptr<FluxProcessor> createProcessor() override;
    std::shared_ptr<Component> createEditor(const NodeEditorContext& ctx) override;

    // --- IHostApplication ---
    Steinberg::tresult PLUGIN_API getName(Steinberg::Vst::String128 name) override;
    Steinberg::tresult PLUGIN_API createInstance(Steinberg::TUID cid, Steinberg::TUID _iid, void** obj) override;
    
    // --- IComponentHandler ---
    Steinberg::tresult PLUGIN_API beginEdit(Steinberg::Vst::ParamID id) override;
    Steinberg::tresult PLUGIN_API performEdit(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue valueNormalized) override;
    Steinberg::tresult PLUGIN_API endEdit(Steinberg::Vst::ParamID id) override;
    Steinberg::tresult PLUGIN_API restartComponent(Steinberg::int32 flags) override;

    // VST3 COM conflict resolution
    Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void** obj) override;
    Steinberg::uint32 PLUGIN_API addRef() override { return 1; }
    Steinberg::uint32 PLUGIN_API release() override { return 1; } 

private:
    std::string m_path;
    std::string m_pluginName = "VST3 Plugin";
    
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace Beam

#endif // VST3_HOST_HPP
