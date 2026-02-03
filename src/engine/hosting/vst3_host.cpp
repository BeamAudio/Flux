#include "engine/hosting/vst3_host.hpp"
#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "interface/widgets/label.hpp"
#include "interface/core/theme.hpp"
#include "interface/editors/vst_external_editor.hpp"
#include <windows.h>
#include <iostream>
#include <vector>

namespace Beam {

using InitModuleFunc = bool (PLUGIN_API*) ();
using ExitModuleFunc = bool (PLUGIN_API*) ();
using GetFactoryFunc = Steinberg::IPluginFactory* (PLUGIN_API*) ();

class VST3ParameterValueQueue : public Steinberg::Vst::IParamValueQueue {
public:
    VST3ParameterValueQueue(Steinberg::Vst::ParamID id) : m_id(id) {}
    Steinberg::Vst::ParamID PLUGIN_API getParameterId() override { return m_id; }
    Steinberg::int32 PLUGIN_API getPointCount() override { return (Steinberg::int32)m_points.size(); }
    Steinberg::tresult PLUGIN_API getPoint(Steinberg::int32 index, Steinberg::int32& sampleOffset, Steinberg::Vst::ParamValue& value) override {
        if (index < 0 || index >= (Steinberg::int32)m_points.size()) return Steinberg::kResultFalse;
        sampleOffset = m_points[index].offset;
        value = m_points[index].value;
        return Steinberg::kResultTrue;
    }
    Steinberg::tresult PLUGIN_API addPoint(Steinberg::int32 sampleOffset, Steinberg::Vst::ParamValue value, Steinberg::int32& index) override {
        m_points.push_back({sampleOffset, value});
        index = (Steinberg::int32)m_points.size() - 1;
        return Steinberg::kResultTrue;
    }
    void clear() { m_points.clear(); }
    Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void** obj) override { return Steinberg::kNoInterface; }
    Steinberg::uint32 PLUGIN_API addRef() override { return 1; }
    Steinberg::uint32 PLUGIN_API release() override { return 1; }
private:
    Steinberg::Vst::ParamID m_id;
    struct Point { Steinberg::int32 offset; Steinberg::Vst::ParamValue value; };
    std::vector<Point> m_points;
};

class VST3ParameterChanges : public Steinberg::Vst::IParameterChanges {
public:
    Steinberg::int32 PLUGIN_API getParameterCount() override { return (Steinberg::int32)m_queues.size(); }
    Steinberg::Vst::IParamValueQueue* PLUGIN_API getParameterData(Steinberg::int32 index) override {
        if (index < 0 || index >= (Steinberg::int32)m_queues.size()) return nullptr;
        return &m_queues[index];
    }
    Steinberg::Vst::IParamValueQueue* PLUGIN_API addParameterData(const Steinberg::Vst::ParamID& id, Steinberg::int32& index) override {
        for (size_t i = 0; i < m_queues.size(); ++i) {
            if (m_queues[i].getParameterId() == id) {
                index = (Steinberg::int32)i;
                return &m_queues[i];
            }
        }
        m_queues.emplace_back(id);
        index = (Steinberg::int32)m_queues.size() - 1;
        return &m_queues.back();
    }
    void clear() { for(auto& q : m_queues) q.clear(); }
    Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void** obj) override { return Steinberg::kNoInterface; }
    Steinberg::uint32 PLUGIN_API addRef() override { return 1; }
    Steinberg::uint32 PLUGIN_API release() override { return 1; }
private:
    std::vector<VST3ParameterValueQueue> m_queues;
};

struct VST3HostNode::Impl {
    HMODULE module = nullptr;
    Steinberg::IPluginFactory* factory = nullptr;
    Steinberg::Vst::IComponent* component = nullptr;
    Steinberg::Vst::IAudioProcessor* processor = nullptr;
    Steinberg::Vst::IEditController* controller = nullptr;
    std::vector<Steinberg::Vst::ParamID> paramIds;

    ~Impl() {
        if (processor) processor->release();
        if (component) {
            component->setActive(false);
            component->terminate();
            component->release();
        }
        if (controller) {
            controller->terminate();
            controller->release();
        }
        if (factory) factory->release();
        if (module) FreeLibrary(module);
    }
};

VST3HostNode::VST3HostNode(const std::string& pluginPath) : m_path(pluginPath) {
    m_impl = std::make_unique<Impl>();
}

VST3HostNode::~VST3HostNode() = default;

bool VST3HostNode::load() {
    m_impl->module = LoadLibraryA(m_path.c_str());
    if (!m_impl->module) return false;

    auto getFactory = (GetFactoryFunc)GetProcAddress(m_impl->module, "GetPluginFactory");
    if (!getFactory) return false;

    m_impl->factory = getFactory();
    if (!m_impl->factory) return false;

    Steinberg::PClassInfo classInfo;
    if (m_impl->factory->getClassInfo(0, &classInfo) == Steinberg::kResultTrue) {
        m_pluginName = (const char*)classInfo.name;

        if (m_impl->factory->createInstance(classInfo.cid, Steinberg::Vst::IComponent_iid, (void**)&m_impl->component) != Steinberg::kResultTrue) return false;
        
        if (m_impl->component->initialize(static_cast<Steinberg::Vst::IHostApplication*>(this)) != Steinberg::kResultTrue) {
             if (m_impl->component->initialize(nullptr) != Steinberg::kResultTrue) return false;
        }

        if (m_impl->component->queryInterface(Steinberg::Vst::IAudioProcessor_iid, (void**)&m_impl->processor) != Steinberg::kResultTrue) return false;
        
        Steinberg::TUID controllerId;
        if (m_impl->component->getControllerClassId(controllerId) == Steinberg::kResultTrue) {
            m_impl->factory->createInstance(controllerId, Steinberg::Vst::IEditController_iid, (void**)&m_impl->controller);
            if (m_impl->controller) {
                m_impl->controller->initialize(static_cast<Steinberg::Vst::IHostApplication*>(this));
                Steinberg::int32 paramCount = m_impl->controller->getParameterCount();
                for (Steinberg::int32 i = 0; i < paramCount; ++i) {
                    Steinberg::Vst::ParameterInfo info;
                    if (m_impl->controller->getParameterInfo(i, info) == Steinberg::kResultTrue) {
                        if (info.flags & Steinberg::Vst::ParameterInfo::kIsReadOnly) continue;
                        auto p = std::make_shared<Parameter>((const char*)info.title, 0.0f, 1.0f, (float)info.defaultNormalizedValue);
                        addParameter(p);
                        m_impl->paramIds.push_back(info.id);
                    }
                }
            }
        }
        m_impl->component->setActive(true);
        return true;
    }
    return false;
}

Steinberg::tresult PLUGIN_API VST3HostNode::getName(Steinberg::Vst::String128 name) {
    const char* hostName = "Beam Audio Flux";
    for(int i=0; i<128; ++i) {
        name[i] = (Steinberg::char16)hostName[i];
        if (hostName[i] == 0) break;
    }
    return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API VST3HostNode::createInstance(Steinberg::TUID cid, Steinberg::TUID _iid, void** obj) {
    return Steinberg::kNoInterface;
}

Steinberg::tresult PLUGIN_API VST3HostNode::queryInterface(const Steinberg::TUID _iid, void** obj) {
    if (memcmp(_iid, Steinberg::Vst::IHostApplication_iid, sizeof(Steinberg::TUID)) == 0 ||
        memcmp(_iid, Steinberg::FUnknown_iid, sizeof(Steinberg::TUID)) == 0) {
        *obj = static_cast<Steinberg::Vst::IHostApplication*>(this);
        return Steinberg::kResultTrue;
    }
    return Steinberg::kNoInterface;
}

std::string VST3HostNode::getName() const { return m_pluginName; }
std::vector<FluxNode::Port> VST3HostNode::getInputPorts() const { return { {"In", 2} }; }
std::vector<FluxNode::Port> VST3HostNode::getOutputPorts() const { return { {"Out", 2} }; }
size_t VST3HostNode::getLatency() const { return m_impl->processor ? m_impl->processor->getLatencySamples() : 0; }

class VST3ProcessorBridge : public FluxProcessor {
public:
    VST3ProcessorBridge(Steinberg::Vst::IAudioProcessor* proc, const std::vector<std::shared_ptr<Parameter>>& params, const std::vector<Steinberg::Vst::ParamID>& ids) 
        : m_proc(proc), m_params(params), m_ids(ids) 
    {
        if (m_proc) {
            Steinberg::Vst::ProcessSetup setup;
            setup.maxSamplesPerBlock = 1024;
            setup.processMode = Steinberg::Vst::kRealtime;
            setup.sampleRate = 44100.0;
            m_proc->setupProcessing(setup);
            m_proc->setProcessing(true);
        }
    }
    ~VST3ProcessorBridge() { if (m_proc) m_proc->setProcessing(false); }

    void process(const float** inputs, float** outputs, int frames) override {
        if (!m_proc) return;
        m_paramChanges.clear();
        for (size_t i = 0; i < m_params.size(); ++i) {
            Steinberg::int32 index;
            auto* queue = m_paramChanges.addParameterData(m_ids[i], index);
            Steinberg::int32 ptIdx;
            queue->addPoint(0, m_params[i]->getValue(), ptIdx);
        }
        Steinberg::Vst::ProcessData data;
        data.numSamples = frames;
        data.symbolicSampleSize = Steinberg::Vst::kSample32;
        data.inputParameterChanges = &m_paramChanges;
        Steinberg::Vst::AudioBusBuffers inBus;
        inBus.numChannels = 2;
        inBus.channelBuffers32 = (Steinberg::Vst::Sample32**)inputs;
        data.numInputs = 1;
        data.inputs = &inBus;
        Steinberg::Vst::AudioBusBuffers outBus;
        outBus.numChannels = 2;
        outBus.channelBuffers32 = (Steinberg::Vst::Sample32**)outputs;
        data.numOutputs = 1;
        data.outputs = &outBus;
        m_proc->process(data);
    }
private:
    Steinberg::Vst::IAudioProcessor* m_proc;
    const std::vector<std::shared_ptr<Parameter>>& m_params;
    const std::vector<Steinberg::Vst::ParamID>& m_ids;
    VST3ParameterChanges m_paramChanges;
};

std::shared_ptr<FluxProcessor> VST3HostNode::createProcessor() {
    if (!m_impl->processor) return nullptr;
    return std::make_shared<VST3ProcessorBridge>(m_impl->processor, getParameterOrder(), m_impl->paramIds);
}

std::shared_ptr<Component> VST3HostNode::createEditor(const NodeEditorContext& ctx) {
    if (m_impl->controller) {
        return std::make_shared<VSTExternalEditor>(m_impl->controller);
    }
    
    auto container = std::make_shared<Component>();
    container->setName("VST_NoController");
    auto label = std::make_shared<Label>("VST HAS NO EDIT CONTROLLER");
    label->setColor(Theme::Red);
    container->addChildComponent(label);
    return container;
}

} // namespace Beam