#ifndef MIDI_EVENT_HPP
#define MIDI_EVENT_HPP

#include <vector>
#include <cstdint>

namespace Beam {

/**
 * @enum MIDIStatus
 * @brief Standard MIDI status bytes.
 */
enum class MIDIStatus : uint8_t {
    NoteOff = 0x80,
    NoteOn = 0x90,
    ControlChange = 0xB0,
    PitchBend = 0xE0
};

/**
 * @struct MIDIEvent
 * @brief A raw MIDI event with a local frame offset.
 */
struct MIDIEvent {
    uint32_t frameOffset; ///< Offset from the start of the current processing block
    uint8_t status;      ///< MIDI Status byte
    uint8_t data1;       ///< MIDI Data 1 (e.g., Note Number)
    uint8_t data2;       ///< MIDI Data 2 (e.g., Velocity)
};

/**
 * @class MIDIBuffer
 * @brief Container for MIDI events within a single processing block.
 */
class MIDIBuffer {
public:
    void addEvent(const MIDIEvent& event) {
        m_events.push_back(event);
    }

    void clear() {
        m_events.clear();
    }

    const std::vector<MIDIEvent>& getEvents() const { return m_events; }

private:
    std::vector<MIDIEvent> m_events;
};

} // namespace Beam

#endif // MIDI_EVENT_HPP

