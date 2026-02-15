#include "engine/midi/midi_device_manager.hpp"
#include <iostream>

namespace Beam {

MidiDeviceManager::MidiDeviceManager() {}
MidiDeviceManager::~MidiDeviceManager() { shutdown(); }

void MidiDeviceManager::init() {
    if (m_initialized) return;
    int numDevs = midiInGetNumDevs();
    std::cout << "[MIDI] Found " << numDevs << " input devices." << std::endl;
    m_initialized = true;
    
    // Auto-open all available inputs for now (Omni-like behavior)
    for (int i = 0; i < numDevs; ++i) {
        openInputDevice(i);
    }
}

void MidiDeviceManager::shutdown() {
    closeAllInputDevices();
    m_initialized = false;
}

std::vector<MidiDeviceInfo> MidiDeviceManager::getAvailableInputDevices() {
    std::vector<MidiDeviceInfo> devices;
    int numDevs = midiInGetNumDevs();
    for (int i = 0; i < numDevs; ++i) {
        MIDIINCAPSA caps;
        if (midiInGetDevCapsA(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
            devices.push_back({i, caps.szPname, true});
        }
    }
    return devices;
}

std::vector<MidiDeviceInfo> MidiDeviceManager::getAvailableOutputDevices() {
    std::vector<MidiDeviceInfo> devices;
    int numDevs = midiOutGetNumDevs();
    for (int i = 0; i < numDevs; ++i) {
        MIDIOUTCAPSA caps;
        if (midiOutGetDevCapsA(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
            devices.push_back({i, caps.szPname, false});
        }
    }
    return devices;
}

bool MidiDeviceManager::openInputDevice(int deviceId) {
    HMIDIIN hMidiIn;
    MMRESULT result = midiInOpen(&hMidiIn, deviceId, (DWORD_PTR)midiInProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
    if (result == MMSYSERR_NOERROR) {
        midiInStart(hMidiIn);
        std::lock_guard<std::mutex> lock(m_mutex);
        m_openInputs.push_back(hMidiIn);
        m_inputMap[hMidiIn] = deviceId;
        std::cout << "[MIDI] Opened Input Device ID: " << deviceId << std::endl;
        return true;
    }
    std::cerr << "[MIDI] Failed to open Input Device ID: " << deviceId << " Error: " << result << std::endl;
    return false;
}

void MidiDeviceManager::closeInputDevice(int deviceId) {
    // Requires finding handle by ID, simplified to close all for now or logic upgrade
}

void MidiDeviceManager::closeAllInputDevices() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto hMidiIn : m_openInputs) {
        midiInStop(hMidiIn);
        midiInClose(hMidiIn);
    }
    m_openInputs.clear();
    m_inputMap.clear();
}

void CALLBACK MidiDeviceManager::midiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    MidiDeviceManager* manager = (MidiDeviceManager*)dwInstance;
    if (!manager) return;

    if (wMsg == MIM_DATA) {
        // Handle Short Message
        unsigned char status = (dwParam1 & 0x000000FF);
        unsigned char data1  = (dwParam1 & 0x0000FF00) >> 8;
        unsigned char data2  = (dwParam1 & 0x00FF0000) >> 16;
        
        // Filter System Realtime (Clock/Start/Stop) for now to reduce noise
        if (status >= 0xF8) return;

        MIDIEvent event;
        event.status = status;
        event.data1 = data1;
        event.data2 = data2;
        event.timestamp = (double)dwParam2 * 0.001; // ms to seconds? dwParam2 is timestamp in ms

        if (manager->m_callback) {
            manager->m_callback(event);
        }
    }
}

} // namespace Beam
