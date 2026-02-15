#include "engine/hosting/vst3_host.hpp"
#include "engine/hosting/vst3_utils.hpp"
#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/base/ibstream.h"
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

// --- Helper Classes ---

class HostMemoryStream : public Steinberg::IBStream {
public:
    HostMemoryStream() : m_pos(0) {}
    HostMemoryStream(const std::vector<char>& data) : m_buffer(data), m_pos(0) {}

    Steinberg::tresult PLUGIN_API read(void* buffer, Steinberg::int32 numBytes, Steinberg::int32* numBytesRead) override {
        if (m_pos >= m_buffer.size()) {
            if (numBytesRead) *numBytesRead = 0;
            return Steinberg::kResultFalse;
        }
        Steinberg::int32 available = (Steinberg::int32)(m_buffer.size() - m_pos);
        Steinberg::int32 toRead = (numBytes < available) ? numBytes : available;
        memcpy(buffer, m_buffer.data() + m_pos, toRead);
        m_pos += toRead;
        if (numBytesRead) *numBytesRead = toRead;
        return Steinberg::kResultTrue;
    }

    Steinberg::tresult PLUGIN_API write(void* buffer, Steinberg::int32 numBytes, Steinberg::int32* numBytesWritten) override {
        size_t endPos = m_pos + numBytes;
        if (endPos > m_buffer.capacity()) m_buffer.reserve(endPos * 2); 
        if (endPos > m_buffer.size()) m_buffer.resize(endPos);
        memcpy(m_buffer.data() + m_pos, buffer, numBytes);
        m_pos += numBytes;
        if (numBytesWritten) *numBytesWritten = numBytes;
        return Steinberg::kResultTrue;
    }

    Steinberg::tresult PLUGIN_API seek(Steinberg::int64 pos, Steinberg::int32 mode, Steinberg::int64* result) override {
        if (mode == Steinberg::IBStream::kIBSeekSet) m_pos = (size_t)pos;
        else if (mode == Steinberg::IBStream::kIBSeekCur) m_pos += (size_t)pos;
        else if (mode == Steinberg::IBStream::kIBSeekEnd) m_pos = m_buffer.size() + (size_t)pos;
        
        if (m_pos > m_buffer.size()) m_pos = m_buffer.size();  
        if (result) *result = m_pos;
        return Steinberg::kResultTrue;
    }

    Steinberg::tresult PLUGIN_API tell(Steinberg::int64* pos) override {
        if (pos) *pos = m_pos;
        return Steinberg::kResultTrue;
    }

    const std::vector<char>& getData() const { return m_buffer; }

    Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void** obj) override { return Steinberg::kNoInterface; }
    Steinberg::uint32 PLUGIN_API addRef() override { return 1; }
    Steinberg::uint32 PLUGIN_API release() override { return 1; }

private:
    std::vector<char> m_buffer;
    size_t m_pos;
};

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
        if (module) {
            if (ModuleLeakRegistry::get().isUnsafe(module)) {
                 std::cerr << "Module Leaked: " << module << std::endl;
            } else {
                 FreeLibrary(module);
            }
        }
    }
};

VST3HostNode::VST3HostNode(const std::string& pluginPath) : m_path(pluginPath) { m_impl = std::make_unique<Impl>(); }
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
        m_impl->component->initialize(static_cast<Steinberg::Vst::IHostApplication*>(this));
        m_impl->component->queryInterface(Steinberg::Vst::IAudioProcessor_iid, (void**)&m_impl->processor);
        m_inputChannels = 2; m_outputChannels = 2;
        Steinberg::TUID controllerId;
        if (m_impl->component->getControllerClassId(controllerId) == Steinberg::kResultTrue) {
            m_impl->factory->createInstance(controllerId, Steinberg::Vst::IEditController_iid, (void**)&m_impl->controller);
            if (m_impl->controller) {
                m_impl->controller->initialize(static_cast<Steinberg::Vst::IHostApplication*>(this));
                m_impl->controller->setComponentHandler(static_cast<Steinberg::Vst::IComponentHandler*>(this));
                Steinberg::int32 paramCount = m_impl->controller->getParameterCount();
                for (Steinberg::int32 i = 0; i < paramCount; ++i) {
                    Steinberg::Vst::ParameterInfo info;
                    if (m_impl->controller->getParameterInfo(i, info) == Steinberg::kResultTrue) {
                        if (info.flags & Steinberg::Vst::ParameterInfo::kIsReadOnly) continue;
                        auto p = std::make_shared<Parameter>((const char*)info.title, 0.0f, 1.0f, (float)info.defaultNormalizedValue);
                        addParameter(p);
                        m_impl->paramIds.push_back(info.id);
                        m_paramMap[info.id] = p;
                    }
                }
            }
        }
        // Connect Component and Controller
        Steinberg::Vst::IConnectionPoint* componentCP = nullptr;
        Steinberg::Vst::IConnectionPoint* controllerCP = nullptr;

        m_impl->component->queryInterface(Steinberg::Vst::IConnectionPoint::iid, (void**)&componentCP);
        m_impl->controller->queryInterface(Steinberg::Vst::IConnectionPoint::iid, (void**)&controllerCP);

        if (componentCP && controllerCP) {
            componentCP->connect(controllerCP);
            controllerCP->connect(componentCP);
            std::cout << "[VST3Host] Connected Component and Controller." << std::endl;
        }

        if (componentCP) componentCP->release();
        if (controllerCP) controllerCP->release();

        m_impl->component->setActive(true);
        return true;
    }
    return false;
}

Steinberg::tresult PLUGIN_API VST3HostNode::getName(Steinberg::Vst::String128 name) {
    const char16_t* hostName = u"Beam Audio Flux";
    for(int i=0; i<128; ++i) { name[i] = (Steinberg::char16)hostName[i]; if (hostName[i] == 0) break; }
    return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API VST3HostNode::beginEdit(Steinberg::Vst::ParamID id) { return Steinberg::kResultTrue; }
Steinberg::tresult PLUGIN_API VST3HostNode::performEdit(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue valueNormalized) {
    auto it = m_paramMap.find(id); if (it != m_paramMap.end()) it->second->setValue((float)valueNormalized);
    return Steinberg::kResultTrue;
}
Steinberg::tresult PLUGIN_API VST3HostNode::endEdit(Steinberg::Vst::ParamID id) { return Steinberg::kResultTrue; }
Steinberg::tresult PLUGIN_API VST3HostNode::restartComponent(Steinberg::int32 flags) { return Steinberg::kResultTrue; }

Steinberg::tresult PLUGIN_API VST3HostNode::createInstance(Steinberg::TUID cid, Steinberg::TUID _iid, void** obj) {
    if (memcmp(cid, Steinberg::Vst::IMessage_iid, sizeof(Steinberg::TUID)) == 0 || memcmp(_iid, Steinberg::Vst::IMessage_iid, sizeof(Steinberg::TUID)) == 0) {
        *obj = new HostMessage(); return Steinberg::kResultTrue;
    }
    return Steinberg::kNoInterface;
}

Steinberg::tresult PLUGIN_API VST3HostNode::setDirty(Steinberg::TBool state) { return Steinberg::kResultTrue; }
Steinberg::tresult PLUGIN_API VST3HostNode::requestOpenEditor(Steinberg::FIDString name) { return Steinberg::kResultTrue; }
Steinberg::tresult PLUGIN_API VST3HostNode::startGroupEdit() { return Steinberg::kResultTrue; }
Steinberg::tresult PLUGIN_API VST3HostNode::finishGroupEdit() { return Steinberg::kResultTrue; }

Steinberg::tresult PLUGIN_API VST3HostNode::queryInterface(const Steinberg::TUID _iid, void** obj) {
    if (Steinberg::FUnknownPrivate::iidEqual(_iid, Steinberg::Vst::IHostApplication::iid) ||
        Steinberg::FUnknownPrivate::iidEqual(_iid, Steinberg::FUnknown::iid)) {
        *obj = static_cast<Steinberg::Vst::IHostApplication*>(this);
        addRef();
        return Steinberg::kResultTrue;
    }
    if (Steinberg::FUnknownPrivate::iidEqual(_iid, Steinberg::Vst::IComponentHandler::iid)) {
        *obj = static_cast<Steinberg::Vst::IComponentHandler*>(this);
        addRef();
        return Steinberg::kResultTrue;
    }
    if (Steinberg::FUnknownPrivate::iidEqual(_iid, Steinberg::Vst::IComponentHandler2::iid)) {
        *obj = static_cast<Steinberg::Vst::IComponentHandler2*>(this);
        addRef();
        return Steinberg::kResultTrue;
    }
    if (Steinberg::FUnknownPrivate::iidEqual(_iid, Steinberg::Vst::IAttributeList::iid)) {
        // Many plugins ask for this when loading presets or initing. Returning null can crash them.
        // We don't implement it fully yet, but returning kNoInterface is safer than a bad pointer,
        // UNLESS the plugin assumes success.
        // For now, let's keep it kNoInterface but log it?
        // Actually, some plugins crash if QueryInterface fails for things they expect.
        // But we can't implement it without a class.
        // Let's leave it as kNoInterface but ensure we don't crash in earlier calls.
        *obj = nullptr;
        return Steinberg::kNoInterface;
    }
    *obj = nullptr;
    return Steinberg::kNoInterface;
}

nlohmann::json VST3HostNode::serialize() const {
    nlohmann::json data = FluxNode::serialize();
    data["path"] = m_path;
    data["type"] = "VST3";
    
    if (m_impl && m_impl->component) {
        HostMemoryStream stream;
        if (m_impl->component->getState(&stream) == Steinberg::kResultTrue) {
            data["chunk"] = stream.getData(); 
        }
    }
    return data;
}

void VST3HostNode::deserialize(const nlohmann::json& data) {
    // FluxNode::deserialize(data); // Don't restore params immediately, let the chunk do it first
    
    if (data.contains("chunk") && m_impl && m_impl->component) {
        try {
            std::vector<char> chunk = data["chunk"].get<std::vector<char>>();
            if (!chunk.empty()) {
                HostMemoryStream stream(chunk);
                if (m_impl->component->setState(&stream) != Steinberg::kResultTrue) {
                    std::cerr << "[VST3] Warning: Failed to restore component state." << std::endl;
                }
            }
        } catch (...) {
            std::cerr << "[VST3] Error: Failed to deserialize state chunk." << std::endl;
        }
    }
    
    // Restore parameters after chunk (optional override from automation)
    FluxNode::deserialize(data);
}

std::string VST3HostNode::getName() const { return m_pluginName; }
std::vector<FluxNode::Port> VST3HostNode::getInputPorts() const { return { {"In", m_inputChannels} }; }
std::vector<FluxNode::Port> VST3HostNode::getOutputPorts() const { return { {"Out", m_outputChannels} }; }
size_t VST3HostNode::getLatency() const { return m_impl->processor ? m_impl->processor->getLatencySamples() : 0; }

class VST3ProcessorBridge : public FluxProcessor {
public:
    VST3ProcessorBridge(Steinberg::Vst::IAudioProcessor* proc, const std::vector<std::shared_ptr<Parameter>>& params, const std::vector<Steinberg::Vst::ParamID>& ids, int inCh, int outCh) 
        : m_proc(proc), m_params(params), m_ids(ids), m_inChannels(inCh), m_outChannels(outCh) 
    {
        if (m_proc) {
            Steinberg::Vst::ProcessSetup setup; setup.maxSamplesPerBlock = 8192; setup.processMode = Steinberg::Vst::kRealtime; setup.sampleRate = 44100.0;
            m_proc->setupProcessing(setup); m_proc->setProcessing(true);
        }
        m_inPlanar.resize(m_inChannels); m_outPlanar.resize(m_outChannels);
        m_inPtrs.resize(m_inChannels); m_outPtrs.resize(m_outChannels);
        for (int ch = 0; ch < m_inChannels; ++ch) m_inPlanar[ch].resize(1024);
        for (int ch = 0; ch < m_outChannels; ++ch) m_outPlanar[ch].resize(1024);
    }
    ~VST3ProcessorBridge() { if (m_proc) m_proc->setProcessing(false); }

    void process(const float** inputs, float** outputs, int frames) override {
        if (!m_proc) return;
        m_paramChanges.clear();
        for (size_t i = 0; i < m_params.size(); ++i) {
            Steinberg::int32 index; auto* queue = m_paramChanges.addParameterData(m_ids[i], index);
            Steinberg::int32 ptIdx; queue->addPoint(0, m_params[i]->getValue(), ptIdx);
        }
        if (inputs[0]) {
            for (int ch = 0; ch < m_inChannels; ++ch) {
                for (int i = 0; i < frames; ++i) m_inPlanar[ch][i] = inputs[0][i * m_inChannels + ch];
            }
        }
        Steinberg::Vst::ProcessData data; data.numSamples = frames; data.symbolicSampleSize = Steinberg::Vst::kSample32;
        data.inputParameterChanges = &m_paramChanges;
        for (int ch = 0; ch < m_inChannels; ++ch) m_inPtrs[ch] = m_inPlanar[ch].data();
        Steinberg::Vst::AudioBusBuffers inBus; inBus.numChannels = m_inChannels; inBus.channelBuffers32 = m_inPtrs.data();
        data.numInputs = 1; data.inputs = &inBus;
        for (int ch = 0; ch < m_outChannels; ++ch) m_outPtrs[ch] = m_outPlanar[ch].data();
        Steinberg::Vst::AudioBusBuffers outBus; outBus.numChannels = m_outChannels; outBus.channelBuffers32 = m_outPtrs.data();
        data.numOutputs = 1; data.outputs = &outBus;
        try { m_proc->process(data); } catch (...) {}
        if (outputs[0]) {
            for (int ch = 0; ch < m_outChannels; ++ch) {
                for (int i = 0; i < frames; ++i) outputs[0][i * m_outChannels + ch] = m_outPlanar[ch][i];
            }
        }
    }
private:
    Steinberg::Vst::IAudioProcessor* m_proc;
    const std::vector<std::shared_ptr<Parameter>>& m_params;
    const std::vector<Steinberg::Vst::ParamID>& m_ids;
    VST3ParameterChanges m_paramChanges;
    int m_inChannels, m_outChannels;
    std::vector<std::vector<float>> m_inPlanar;
    std::vector<std::vector<float>> m_outPlanar;
    std::vector<float*> m_inPtrs;
    std::vector<float*> m_outPtrs;
};

std::shared_ptr<FluxProcessor> VST3HostNode::createProcessor() {
    if (!m_impl->processor) return nullptr;
    return std::make_shared<VST3ProcessorBridge>(m_impl->processor, getParameterOrder(), m_impl->paramIds, m_inputChannels, m_outputChannels);
}

std::shared_ptr<Component> VST3HostNode::createEditor(const NodeEditorContext& ctx) {
    if (m_impl->controller) return std::make_shared<VSTExternalEditor>(m_impl->controller, ctx.nativeWindowHandle, m_impl->module);
    return std::make_shared<Component>();
}

} // namespace Beam
