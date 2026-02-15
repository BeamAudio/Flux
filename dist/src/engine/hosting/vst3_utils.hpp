#ifndef VST3_UTILS_HPP
#define VST3_UTILS_HPP

#include "pluginterfaces/vst/ivstattributes.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <mutex>
#include <unordered_set>
#include <windows.h>

namespace Beam {

/**
 * @class HostAttributeList
 * @brief Simple implementation of IAttributeList for VST3 host internal messages.
 */
class HostAttributeList : public Steinberg::Vst::IAttributeList {
public:
    Steinberg::tresult PLUGIN_API setInt(AttrID id, Steinberg::int64 value) override { m_ints[id] = value; return Steinberg::kResultTrue; }
    Steinberg::tresult PLUGIN_API getInt(AttrID id, Steinberg::int64& value) override { 
        auto it = m_ints.find(id);
        if (it != m_ints.end()) { value = it->second; return Steinberg::kResultTrue; }
        return Steinberg::kResultFalse;
    }
    Steinberg::tresult PLUGIN_API setFloat(AttrID id, double value) override { m_floats[id] = value; return Steinberg::kResultTrue; }
    Steinberg::tresult PLUGIN_API getFloat(AttrID id, double& value) override {
        auto it = m_floats.find(id);
        if (it != m_floats.end()) { value = it->second; return Steinberg::kResultTrue; }
        return Steinberg::kResultFalse;
    }
    Steinberg::tresult PLUGIN_API setString(AttrID id, const Steinberg::Vst::TChar* string) override { m_strings[id] = string; return Steinberg::kResultTrue; }
    Steinberg::tresult PLUGIN_API getString(AttrID id, Steinberg::Vst::TChar* string, Steinberg::uint32 sizeInBytes) override { return Steinberg::kResultFalse; }
    Steinberg::tresult PLUGIN_API setBinary(AttrID id, const void* data, Steinberg::uint32 sizeInBytes) override { return Steinberg::kResultFalse; }
    Steinberg::tresult PLUGIN_API getBinary(AttrID id, const void*& data, Steinberg::uint32& sizeInBytes) override { return Steinberg::kResultFalse; }

    Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void** obj) override { return Steinberg::kNoInterface; }
    Steinberg::uint32 PLUGIN_API addRef() override { return 1; }
    Steinberg::uint32 PLUGIN_API release() override { return 1; }

private:
    std::map<std::string, Steinberg::int64> m_ints;
    std::map<std::string, double> m_floats;
    std::map<std::string, std::basic_string<Steinberg::Vst::TChar>> m_strings;
};

/**
 * @class HostMessage
 * @brief Simple implementation of IMessage for VST3 host internal communication.
 */
class HostMessage : public Steinberg::Vst::IMessage {
public:
    HostMessage() : m_attributes(new HostAttributeList()) {}
    ~HostMessage() { m_attributes->release(); }

    Steinberg::FIDString PLUGIN_API getMessageID() override { return m_messageId.c_str(); }
    void PLUGIN_API setMessageID(Steinberg::FIDString id) override { m_messageId = id; }
    Steinberg::Vst::IAttributeList* PLUGIN_API getAttributes() override { m_attributes->addRef(); return m_attributes; }

    Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void** obj) override { return Steinberg::kNoInterface; }
    Steinberg::uint32 PLUGIN_API addRef() override { return 1; }
    Steinberg::uint32 PLUGIN_API release() override { return 1; }

private:
    std::string m_messageId;
    HostAttributeList* m_attributes;
};

/**
 * @class ModuleLeakRegistry
 * @brief Singleton to track VST modules that are unsafe to unload due to leaked threads.
 */
class ModuleLeakRegistry {
public:
    static ModuleLeakRegistry& get() {
        static ModuleLeakRegistry instance;
        return instance;
    }

    void markUnsafe(HMODULE handle) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_unsafeModules.insert(handle);
    }

    bool isUnsafe(HMODULE handle) {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_unsafeModules.count(handle) > 0;
    }

private:
    ModuleLeakRegistry() = default;
    std::mutex m_mutex;
    std::unordered_set<HMODULE> m_unsafeModules;
};

} // namespace Beam

#endif // VST3_UTILS_HPP
