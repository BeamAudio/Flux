#ifndef PIANO_ROLL_HPP
#define PIANO_ROLL_HPP

#include "interface/core/component.hpp"
#include "interface/core/theme.hpp"
#include "engine/midi/midi_event.hpp"
#include <vector>
#include <algorithm>

namespace Beam {

/**
 * @class PianoRoll
 * @brief MIDI sequence editor with vertical keys and horizontal time grid.
 */
class PianoRoll : public Component {
public:
    PianoRoll(std::shared_ptr<MIDISequence> sequence) : m_sequence(sequence) {
        setName("PianoRoll");
        m_zoomX = 1.0f;
        m_zoomY = 1.0f;
        m_offsetY = 60 * 20; // Start middle C area
    }

    void paint(QuadBatcher& batcher) override {
        // Background
        batcher.drawQuad(0, 0, m_bounds.w, m_bounds.h, 0.08f, 0.08f, 0.1f, 1.0f);

        float keyW = 60.0f;
        float noteH = 20.0f * m_zoomY;
        float pixelsPerSecond = 100.0f * m_zoomX;
        float framesPerPixel = 44100.0f / pixelsPerSecond;

        // 1. Horizontal Grid (Pitch)
        for (int n = 0; n < 128; ++n) {
            float ny = m_bounds.h - (n + 1) * noteH + m_offsetY;
            if (ny + noteH < 0 || ny > m_bounds.h) continue;

            bool isBlack = false;
            int pc = n % 12;
            if (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10) isBlack = true;

            float r = isBlack ? 0.05f : 0.12f;
            batcher.drawQuad(keyW, ny, m_bounds.w - keyW, noteH, r, r, r + 0.02f, 1.0f);
            batcher.drawQuad(keyW, ny, m_bounds.w - keyW, 1, 0.2f, 0.2f, 0.25f, 0.3f);
        }

        // 2. Vertical Grid (Time)
        float beatW = pixelsPerSecond * 0.5f; // 120 BPM = 0.5s per beat
        for (float x = keyW - m_offsetX; x < m_bounds.w; x += beatW) {
            if (x < keyW) continue;
            batcher.drawQuad(x, 0, 1, m_bounds.h, 0.25f, 0.25f, 0.3f, 0.4f);
        }

        // 3. Notes
        if (m_sequence) {
            for (const auto& note : m_sequence->getNotes()) {
                float nx = keyW + (float)note.startFrame / framesPerPixel - m_offsetX;
                float nw = (float)note.duration / framesPerPixel;
                float ny = m_bounds.h - (note.noteNumber + 1) * noteH + m_offsetY;
                float nh = noteH - 2;

                if (nx + nw < keyW || nx > m_bounds.w) continue;
                
                Color noteCol = Theme::Emerald;
                if (m_selectedNoteId == note.id) noteCol = Color(0.4f, 0.8f, 1.0f);

                batcher.drawBeveledRect(nx, ny + 1, nw, nh, 2.0f, 0.5f, noteCol.r, noteCol.g, noteCol.b, 1.0f);
            }
        }

        // 4. Piano Keys (Left Sidebar)
        batcher.drawQuad(0, 0, keyW, m_bounds.h, 0.15f, 0.15f, 0.18f, 1.0f);
        for (int n = 0; n < 128; ++n) {
            float ny = m_bounds.h - (n + 1) * noteH + m_offsetY;
            if (ny + noteH < 0 || ny > m_bounds.h) continue;

            int pc = n % 12;
            bool isBlack = (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10);
            
            if (isBlack) {
                batcher.drawBeveledRect(0, ny, keyW * 0.6f, noteH, 1.0f, 0.5f, 0.02f, 0.02f, 0.02f, 1.0f);
            } else {
                batcher.drawBeveledRect(0, ny, keyW, noteH, 1.0f, 0.5f, 0.95f, 0.95f, 0.95f, 1.0f);
                if (pc == 0) { // Mark C notes
                    char buf[8]; snprintf(buf, 8, "C%d", n/12 - 1);
                    batcher.drawText(buf, keyW - 20, ny + 4, 9, 0.4f, 0.4f, 0.4f, 1.0f);
                }
            }
        }
    }

    bool onMouseDown(float x, float y, int button, bool shift) override {
        float lx = x - m_bounds.x;
        float ly = y - m_bounds.y;
        float keyW = 60.0f;

        if (lx < keyW) return true; // Clicked keys

        float pixelsPerSecond = 100.0f * m_zoomX;
        float framesPerPixel = 44100.0f / pixelsPerSecond;
        float noteH = 20.0f * m_zoomY;

        size_t frame = (size_t)((lx - keyW + m_offsetX) * framesPerPixel);
        int noteNum = (int)((m_bounds.h - ly + m_offsetY) / noteH);

        // Hit test existing notes
        m_selectedNoteId = (size_t)-1;
        if (m_sequence) {
            for (auto& note : m_sequence->getNotes()) {
                float nx = keyW + (float)note.startFrame / framesPerPixel - m_offsetX;
                float nw = (float)note.duration / framesPerPixel;
                float ny = m_bounds.h - (note.noteNumber + 1) * noteH + m_offsetY;
                if (lx >= nx && lx <= nx + nw && ly >= ny && ly <= ny + noteH) {
                    m_selectedNoteId = note.id;
                    m_isDraggingNote = true;
                    m_dragStartFrame = note.startFrame;
                    m_dragStartMouseX = lx;
                    return true;
                }
            }

            // Create new note
            MIDINote newNote;
            newNote.noteNumber = (uint8_t)std::clamp(noteNum, 0, 127);
            newNote.startFrame = frame;
            newNote.duration = (size_t)(44100.0f * 0.25f); // 16th note approx
            newNote.velocity = 100;
            newNote.id = m_sequence->getNextId();
            m_sequence->addNote(newNote);
            m_selectedNoteId = newNote.id;
        }

        return true;
    }

    bool onMouseMove(float x, float y, bool shift) override {
        if (m_isDraggingNote && m_selectedNoteId != (size_t)-1 && m_sequence) {
            float lx = x - m_bounds.x;
            float pixelsPerSecond = 100.0f * m_zoomX;
            float framesPerPixel = 44100.0f / pixelsPerSecond;
            
            float deltaX = lx - m_dragStartMouseX;
            int64_t newFrame = (int64_t)m_dragStartFrame + (int64_t)(deltaX * framesPerPixel);
            
            for (auto& note : m_sequence->getNotes()) {
                if (note.id == m_selectedNoteId) {
                    note.startFrame = (size_t)(std::max)((int64_t)0, newFrame);
                    break;
                }
            }
        }
        return Component::onMouseMove(x, y, shift);
    }

    bool onMouseUp(float x, float y, int button, bool shift) override {
        m_isDraggingNote = false;
        return true;
    }

    bool onMouseWheel(float x, float y, float delta, bool shift) override {
        if (shift) m_zoomX *= (delta > 0 ? 1.1f : 0.9f);
        else m_offsetY += delta * 20.0f;
        return true;
    }

private:
    std::shared_ptr<MIDISequence> m_sequence;
    float m_offsetX = 0, m_offsetY = 0;
    float m_zoomX = 1.0f, m_zoomY = 1.0f;
    size_t m_selectedNoteId = (size_t)-1;
    bool m_isDraggingNote = false;
    size_t m_dragStartFrame = 0;
    float m_dragStartMouseX = 0;
};

} // namespace Beam

#endif
