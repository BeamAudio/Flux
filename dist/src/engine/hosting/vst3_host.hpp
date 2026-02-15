#ifndef VST3_HOST_HPP
#define VST3_HOST_HPP

#include "engine/core/flux_node.hpp"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include <unordered_map>

namespace Beam {

/**
 * @class VST3HostNode
 * @brief Hosts a VST3 plugin.
 */
class VST3HostNode : public FluxNode, public Steinberg::Vst::IHostApplication, public Steinberg::Vst::IComponentHandler, public Steinberg::Vst::IComponentHandler2 {
public:
    VST3HostNode(const std::string& pluginPath);
    ~VST3HostNode() override;

    // FluxNode overrides
    std::shared_ptr<FluxProcessor> createProcessor() override;
    std::string getName() const override;
    std::vector<Port> getInputPorts() const override;
    std::vector<Port> getOutputPorts() const override;
    size_t getLatency() const override;
    std::shared_ptr<Component> createEditor(const NodeEditorContext& ctx) override;

    bool load();

    // Serialization
    nlohmann::json serialize() const override;
    void deserialize(const nlohmann::json& data) override;

    // IHostApplication
    Steinberg::tresult PLUGIN_API getName(Steinberg::Vst::String128 name) override;
    Steinberg::tresult PLUGIN_API createInstance(Steinberg::TUID cid, Steinberg::TUID _iid, void** obj) override;

    // IComponentHandler
    Steinberg::tresult PLUGIN_API beginEdit(Steinberg::Vst::ParamID id) override;
    Steinberg::tresult PLUGIN_API performEdit(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue valueNormalized) override;
    Steinberg::tresult PLUGIN_API endEdit(Steinberg::Vst::ParamID id) override;
    Steinberg::tresult PLUGIN_API restartComponent(Steinberg::int32 flags) override;

    // IComponentHandler2
    Steinberg::tresult PLUGIN_API setDirty(Steinberg::TBool state) override;
    Steinberg::tresult PLUGIN_API requestOpenEditor(Steinberg::FIDString name) override;
    Steinberg::tresult PLUGIN_API startGroupEdit() override;
    Steinberg::tresult PLUGIN_API finishGroupEdit() override;

    // FUnknown
    Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void** obj) override;
    Steinberg::uint32 PLUGIN_API addRef() override { return 1; }
    Steinberg::uint32 PLUGIN_API release() override { return 1; }

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    std::unordered_map<Steinberg::Vst::ParamID, std::shared_ptr<Parameter>> m_paramMap;
    std::string m_path;
    std::string m_pluginName;
    int m_inputChannels = 2;
    int m_outputChannels = 2;
};

} // namespace Beam

#endif // VST3_HOST_HPP
