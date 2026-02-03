#ifndef TAPE_REEL_HPP
#define TAPE_REEL_HPP

#include <SDL3/SDL.h>
#include "interface/modules/audio_module.hpp"
#include "interface/widgets/knob.hpp"
#include "interface/core/theme.hpp"
#include <cmath>
#include <algorithm>
#include "engine/nodes/flux_track_node.hpp"
#include "engine/dsp/flux_audio_utils.hpp"

namespace Beam {

class TapeReel : public AudioModule {
public:
    TapeReel(std::shared_ptr<FluxTrackNode> track, size_t nodeId, float x, float y) 
        : AudioModule(track, nodeId, x, y), m_trackNode(track) {
        setName("TapeReel");
        
        // Remove auto-generated Generic Editor
        if (m_editorComponent) {
            removeChildComponent(m_editorComponent.get());
            m_editorComponent = nullptr;
        }

        // Add Custom Knobs
        m_driveKnob = std::make_shared<Knob>("Drive", 0.0f, 1.0f, 0.0f);
        m_driveKnob->bindParameter(m_trackNode->getParameter("Tape Drive"));
        m_driveKnob->setName("Drive");
        addChildComponent(m_driveKnob);

        m_ageKnob = std::make_shared<Knob>("Age", 0.0f, 1.0f, 0.0f);
        m_ageKnob->bindParameter(m_trackNode->getParameter("Tape Age"));
        m_ageKnob->setName("Age");
        addChildComponent(m_ageKnob);

        // Custom Layout Size
        float w = 240.0f; 
        float h = 200.0f;
        setBounds(x, y, w, h);
    }

    void resized() override {
        AudioModule::resized(); 
        
        float knobSize = 30.0f;
        float yPos = m_bounds.h - 50.0f; 
        float spacing = 20.0f;
        float startX = (m_bounds.w - (knobSize * 2 + spacing)) / 2.0f;

        m_driveKnob->setBounds(startX, yPos, knobSize, knobSize);
        m_ageKnob->setBounds(startX + knobSize + spacing, yPos, knobSize, knobSize);
    }

    void update(float dt) override {
        Component::update(dt);
        auto track = m_trackNode->getInternalNode();
        if (track->getState() == TrackState::Playing) {
            m_rotation += 2.0f * dt; 
            m_scrollTimer += dt * 0.1f;
            if (m_scrollTimer > 1.0f) m_scrollTimer -= 1.0f;
        }
    }

    void paint(QuadBatcher& batcher) override {
        float w = m_bounds.w;
        float h = m_bounds.h;
        
        // 1. Chassis
        batcher.drawChassisPanel(0, 0, w, h, 8.0f, 0.85f, 0.83f, 0.80f, 1.0f);
        
        // 2. Dark Panel
        float transportH = 50.0f;
        batcher.drawRoundedRect(10, h - transportH - 10, w - 20, transportH, 4.0f, 0.5f, 0.2f, 0.2f, 0.2f, 1.0f);

        // 3. Reels Area
        float reelAreaH = h - transportH - 20;
        float reelSize = reelAreaH * 0.85f;
        float reelY = 15.0f;
        
        // Emptiness check
        bool isEmpty = (m_trackNode->getInternalNode()->getTotalFrames() == 0);
        
        auto drawReel = [&](float cx, float cy, float size, bool supply) {
            float fullR = size * 0.45f;
            float packR = fullR * (supply ? (1.0f - m_scrollTimer) : (0.2f + m_scrollTimer));
            if (isEmpty) packR = size * 0.15f; 
            else packR = (std::max)(packR, size * 0.15f);
            
            batcher.drawRoundedRect(cx - size/2, cy - size/2, size, size, size*0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.2f);
            batcher.drawRoundedRect(cx - packR, cy - packR, packR*2, packR*2, packR, 0.5f, 0.35f, 0.2f, 0.15f, 1.0f);
            
            for (int i = 0; i < 3; ++i) {
                float angle = m_rotation + (i * 2.094f);
                float rOut = size * 0.48f;
                float rIn = size * 0.15f;
                float x1 = cx + std::sin(angle) * rIn;
                float y1 = cy - std::cos(angle) * rIn;
                float x2 = cx + std::sin(angle) * rOut;
                float y2 = cy - std::cos(angle) * rOut;
                batcher.drawLine(x1, y1, x2, y2, 3.0f, 0.8f, 0.8f, 0.85f, 1.0f);
            }
            batcher.drawRoundedGradientRect(cx - size*0.1f, cy - size*0.1f, size*0.2f, size*0.2f, size*0.1f, 0.5f, 
                                            0.2f, 0.2f, 0.2f, 1.0f, 0.1f, 0.1f, 0.1f, 1.0f);
        };

        drawReel(w * 0.3f, reelY + reelSize/2, reelSize, true);
        drawReel(w * 0.7f, reelY + reelSize/2, reelSize, false);
        
        // 4. Head Cover
        float headW = w * 0.2f;
        float headH = 30.0f;
        batcher.drawRoundedGradientRect((w - headW)/2, reelY + reelSize - 10, headW, headH, 2.0f, 0.5f, 
                                       0.3f, 0.3f, 0.3f, 1.0f, 0.15f, 0.15f, 0.15f, 1.0f);

        // 5. Labels
        if (m_isRenaming) {
            float tx = w * 0.5f - 60.0f;
            batcher.drawQuad(tx, 5, 120, 20, 1.0f, 1.0f, 1.0f, 1.0f); 
            batcher.drawText(m_tempName + "|", tx + 5, 8, 10.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        } else {
            std::string name = m_trackNode->getName();
            if (name.length() > 15) name = name.substr(0, 12) + "...";
            batcher.drawText(name, w * 0.5f - (name.length() * 3), 5, 10.0f, 0.1f, 0.1f, 0.1f, 0.8f);
        }
        
        float knobY = h - 12.0f;
        batcher.drawText("DRIVE", m_driveKnob->getX() + 2, knobY, 9.0f, 0.6f, 0.6f, 0.6f, 1.0f);
        batcher.drawText("AGE", m_ageKnob->getX() + 6, knobY, 9.0f, 0.6f, 0.6f, 0.6f, 1.0f);

        // Record LED
        float btnSize = 10.0f;
        auto track = m_trackNode->getInternalNode();
        bool isRec = (track->getState() == TrackState::Recording);
        batcher.drawText("REC", w - 45, 10, 10.0f, 0.6f, 0.6f, 0.6f, 1.0f);
        batcher.drawRoundedRect(w - 20, 10, btnSize, btnSize, btnSize*0.5f, 0.5f, isRec ? 1.0f : 0.3f, 0.0f, 0.0f, 1.0f);
    }

    bool onMouseDown(float x, float y, int button, bool shift) override {
        float lx = x - m_bounds.x; 
        float ly = y - m_bounds.y;
        
        if (m_isRenaming) {
             m_isRenaming = false;
             m_trackNode->setName(m_tempName);
             setName(m_tempName);
             return true;
        }

        if (ly < 30.0f && lx > m_bounds.w * 0.2f && lx < m_bounds.w * 0.8f) {
            Uint64 now = SDL_GetTicks();
            if (now - m_lastClickTime < 400) {
                m_isRenaming = true;
                m_tempName = m_trackNode->getName();
                return true;
            }
            m_lastClickTime = now;
        }

        float btnSize = 10.0f;
        Rect recBounds = { m_bounds.w - 20, 10, btnSize, btnSize };
        
        if (recBounds.contains(lx, ly)) {
            auto track = m_trackNode->getInternalNode();
            if (track->getState() == TrackState::Recording) {
                m_trackNode->stopRecording();
            } else {
                std::string path = "recording_" + std::to_string(getNodeId()) + ".wav";
                m_trackNode->startRecording(path, 44100);
            }
            return true;
        }
        return AudioModule::onMouseDown(x, y, button, shift);
    }

    bool onKeyDown(int key) override {
        if (m_isRenaming) {
            if (key == SDLK_RETURN || key == SDLK_RETURN2) {
                m_isRenaming = false;
                m_trackNode->setName(m_tempName);
                setName(m_tempName);
            } else if (key == SDLK_ESCAPE) {
                m_isRenaming = false;
            } else if (key == SDLK_BACKSPACE) {
                if (!m_tempName.empty()) m_tempName.pop_back();
            }
            return true;
        }
        return false;
    }

    bool onTextInput(const std::string& text) override {
        if (m_isRenaming) {
            m_tempName += text;
            return true;
        }
        return false;
    }

private:
    std::shared_ptr<FluxTrackNode> m_trackNode;
    std::shared_ptr<Knob> m_driveKnob;
    std::shared_ptr<Knob> m_ageKnob;
    float m_rotation = 0.0f;
    float m_scrollTimer = 0.0f;
    
    bool m_isRenaming = false;
    std::string m_tempName;
    Uint64 m_lastClickTime = 0;
};

} // namespace Beam

#endif