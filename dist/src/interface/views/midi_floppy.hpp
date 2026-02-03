#ifndef MIDI_FLOPPY_HPP
#define MIDI_FLOPPY_HPP

#include "interface/modules/audio_module.hpp"
#include "engine/nodes/midi_track_node.hpp"
#include "interface/core/theme.hpp"

namespace Beam {

class MidiFloppyUI : public AudioModule {
public:
    MidiFloppyUI(std::shared_ptr<MIDITrackNode> node, size_t nodeId, float x, float y) 
        : AudioModule(node, nodeId, x, y), m_midiNode(node) {
        setName("MidiFloppy");
        
        if (m_editorComponent) {
            removeChildComponent(m_editorComponent.get());
            m_editorComponent = nullptr;
        }

        // Floppy Disk Size (approx)
        float w = 160.0f; 
        float h = 160.0f;
        setBounds(x, y, w, h);
    }

    void paint(QuadBatcher& batcher) override {
        float w = m_bounds.w;
        float h = m_bounds.h;
        
        // 1. Plastic Shell (Blue/Grey)
        batcher.drawRoundedRect(0, 0, w, h, 4.0f, 0.5f, 0.2f, 0.3f, 0.6f, 1.0f);
        
        // 2. Shutter (Metal)
        float shutterH = 60.0f;
        batcher.drawRoundedRect(10, 0, w - 20, shutterH, 2.0f, 0.5f, 0.7f, 0.7f, 0.7f, 1.0f);
        
        // 3. Label Area
        batcher.drawRoundedRect(20, h - 80, w - 40, 60, 2.0f, 0.5f, 0.9f, 0.9f, 0.85f, 1.0f);
        
        // 4. Text
        batcher.drawText("MIDI DATA", 30, h - 70, 12.0f, 0.1f, 0.1f, 0.1f, 1.0f);
        batcher.drawText("Events: " + std::to_string(m_midiNode->getEventCount()), 30, h - 50, 10.0f, 0.1f, 0.1f, 0.1f, 1.0f);

        // 5. Write Protect Tab (Red if recording)
        bool isRec = m_midiNode->isRecording();
        batcher.drawRoundedRect(w - 20, h - 20, 10, 10, 2.0f, 0.5f, isRec ? 1.0f : 0.1f, 0.0f, 0.0f, 1.0f);
    }

private:
    std::shared_ptr<MIDITrackNode> m_midiNode;
};

} // namespace Beam

#endif // MIDI_FLOPPY_HPP
