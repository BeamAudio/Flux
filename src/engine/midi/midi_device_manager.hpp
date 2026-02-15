#ifndef MIDI_DEVICE_MANAGER_HPP
#define MIDI_DEVICE_MANAGER_HPP

#include <vector>
#include <string>
#include <functional>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <windows.h>
#include <mmeapi.h> // Windows MIDI API
#include "engine/midi/midi_event.hpp"

namespace Beam {

struct MidiDeviceInfo {
    int id;
    std::string name;
    bool isInput;
};

class MidiDeviceManager {
public:
    static MidiDeviceManager& get() {
        static MidiDeviceManager instance;
        return instance;
    }

    // Initialize and scan devices
    void init();
    void shutdown();

    std::vector<MidiDeviceInfo> getAvailableInputDevices();
    std::vector<MidiDeviceInfo> getAvailableOutputDevices();

    // Open a device for input
    bool openInputDevice(int deviceId);
    void closeInputDevice(int deviceId);
    void closeAllInputDevices();

    // Callback for incoming MIDI events
    using MidiCallback = std::function<void(const MIDIEvent&)>;
    void setMidiCallback(MidiCallback callback) { m_callback = callback; }

    // Helper: Static callback for Windows API
    static void CALLBACK midiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

private:
    MidiDeviceManager();
    ~MidiDeviceManager();

    std::vector<HMIDIIN> m_openInputs;
    std::map<HMIDIIN, int> m_inputMap; // Handle -> ID
    MidiCallback m_callback;
    std::mutex m_mutex;
    bool m_initialized = false;
};

} // namespace Beam

#endif // MIDI_DEVICE_MANAGER_HPP
