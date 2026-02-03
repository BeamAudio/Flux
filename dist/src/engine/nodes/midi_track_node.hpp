#ifndef MIDI_TRACK_NODE_HPP
#define MIDI_TRACK_NODE_HPP

#include "engine/core/flux_node.hpp"
#include "engine/midi/midi_event.hpp"
#include "interface/widgets/piano_roll.hpp"
#include <vector>
#include <mutex>
#include <atomic>

namespace Beam {

/**
 * @class MIDITrackNode
 * @brief Records and plays back MIDI events. The "MIDI Floppy".
 */
class MIDITrackProcessor : public FluxProcessor {
public:
    MIDITrackProcessor(std::shared_ptr<MIDISequence> sequence, std::atomic<bool>* recording, std::atomic<size_t>* playhead) 
        : m_sequence(sequence), m_isRecording(recording), m_playhead(playhead) {}

    void process(const float** inputs, float** outputs, int frames) override {
        if (!*m_isRecording) {
            m_currentFrame = m_playhead->load();
            m_playhead->fetch_add(frames);
        }
    }

    void processMIDI(MIDIBuffer& midi) override {
        if (*m_isRecording) {
            // Note: Recording logic would need to translate raw events to MIDINote
        } else if (m_sequence) {
            size_t start = m_currentFrame;
            size_t end = start + 512; 
            
            for (const auto& note : m_sequence->getNotes()) {
                // Note On
                if (note.startFrame >= start && note.startFrame < end) {
                    MIDIEvent on;
                    on.frameOffset = (uint32_t)(note.startFrame - start);
                    on.status = (uint8_t)MIDIStatus::NoteOn;
                    on.data1 = note.noteNumber;
                    on.data2 = note.velocity;
                    midi.addEvent(on);
                }
                // Note Off
                size_t offFrame = note.startFrame + note.duration;
                if (offFrame >= start && offFrame < end) {
                    MIDIEvent off;
                    off.frameOffset = (uint32_t)(offFrame - start);
                    off.status = (uint8_t)MIDIStatus::NoteOff;
                    off.data1 = note.noteNumber;
                    off.data2 = 0;
                    midi.addEvent(off);
                }
            }
        }
    }

private:
    std::shared_ptr<MIDISequence> m_sequence;
    std::atomic<bool>* m_isRecording;
    std::atomic<size_t>* m_playhead;
    size_t m_currentFrame = 0;
};

class MIDITrackNode : public FluxNode {
public:
    MIDITrackNode() : m_isRecording(false), m_playhead(0) {
        m_sequence = std::make_shared<MIDISequence>();
    }

    std::shared_ptr<FluxProcessor> createProcessor() override {
        return std::make_shared<MIDITrackProcessor>(m_sequence, &m_isRecording, &m_playhead);
    }

    std::shared_ptr<Component> createEditor(const NodeEditorContext& ctx) override {
        return std::make_shared<PianoRoll>(m_sequence);
    }

    void startRecording() { m_isRecording = true; m_playhead = 0; m_sequence->getNotes().clear(); }
    void stopRecording() { m_isRecording = false; }
    void seek(size_t frame) { m_playhead = frame; }
    
    bool isRecording() const { return m_isRecording.load(); }

    std::string getName() const override { return "MIDI Floppy"; }
    std::vector<FluxNode::Port> getInputPorts() const override { return {{"MIDI In", 0}}; }
    std::vector<FluxNode::Port> getOutputPorts() const override { return {{"MIDI Out", 0}}; }

private:
    std::shared_ptr<MIDISequence> m_sequence;
    std::atomic<bool> m_isRecording;
    std::atomic<size_t> m_playhead;
};

} // namespace Beam
#endif // MIDI_TRACK_NODE_HPP
