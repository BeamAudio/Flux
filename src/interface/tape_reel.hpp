#ifndef TAPE_REEL_HPP
#define TAPE_REEL_HPP

#include "audio_module.hpp"
#include "../engine/flux_track_node.hpp"

namespace Beam {

class TapeReel : public AudioModule {
public:
    TapeReel(std::shared_ptr<FluxTrackNode> track, size_t nodeId, float x, float y) 
        : AudioModule(track, nodeId, x, y), m_trackNode(track) {
        setBounds(x, y, 200, 120);
    }

    void update(float dt) override {
        auto track = m_trackNode->getInternalNode();
        if (track->getState() == TrackState::Playing) {
            m_rotation += 2.0f * dt; 
        }
    }

    void render(QuadBatcher& batcher) override {
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 12.0f, 1.0f, 0.22f, 0.22f, 0.23f, 1.0f);
        auto track = m_trackNode->getInternalNode();
        float reelSize = 65.0f;
        float reelY = m_bounds.y + 35;
        
        auto drawReel = [&](float x, float y) {
            batcher.drawRoundedRect(x, y, reelSize, reelSize, reelSize * 0.5f, 1.0f, 0.6f, 0.61f, 0.63f, 1.0f);
            for (int i = 0; i < 3; ++i) {
                float angle = m_rotation + (i * 2.094f);
                float sx = x + reelSize * 0.5f + std::sin(angle) * reelSize * 0.35f;
                float sy = y + reelSize * 0.5f - std::cos(angle) * reelSize * 0.35f;
                batcher.drawRoundedRect(sx - 4, sy - 4, 8, 8, 4.0f, 0.5f, 0.1f, 0.1f, 0.1f, 1.0f);
            }
            batcher.drawRoundedRect(x + reelSize * 0.5f - 5, y + reelSize * 0.5f - 5, 10, 10, 5.0f, 0.5f, 0.15f, 0.15f, 0.16f, 1.0f);
        };

        drawReel(m_bounds.x + 20, reelY);
        drawReel(m_bounds.x + 115, reelY);
        batcher.drawText(m_trackNode->getName(), m_bounds.x + 15, m_bounds.y + 10, 12, 0.9f, 0.9f, 0.9f, 1.0f);

        // Record Button UI
        Rect recBounds = { m_bounds.x + m_bounds.w - 30, m_bounds.y + 8, 20, 20 };
        if (track->getState() == TrackState::Recording) {
            batcher.drawRoundedRect(recBounds.x, recBounds.y, recBounds.w, recBounds.h, 10.0f, 2.0f, 1.0f, 0.1f, 0.1f, 1.0f); // Glowing red
            batcher.drawText("REC", m_bounds.x + m_bounds.w - 55, m_bounds.y + 12, 10, 1.0f, 0.2f, 0.2f, 1.0f);
        } else {
            batcher.drawRoundedRect(recBounds.x, recBounds.y, recBounds.w, recBounds.h, 10.0f, 0.5f, 0.3f, 0.05f, 0.05f, 1.0f); // Dim red
        }

        // Parent's ports
        getInputPort()->render(batcher);
        getOutputPort()->render(batcher);
    }

    bool onMouseDown(float x, float y, int button) override {
        if (AudioModule::onMouseDown(x, y, button)) return true;
        auto track = m_trackNode->getInternalNode();
        
        // Record Button check
        Rect recBounds = { m_bounds.x + m_bounds.w - 30, m_bounds.y + 8, 20, 20 };
        if (recBounds.contains(x, y)) {
            if (track->getState() == TrackState::Recording) {
                m_trackNode->stopRecording();
            } else {
                std::string path = "recording_" + std::to_string(m_nodeId) + ".wav";
                m_trackNode->startRecording(path, 44100);
            }
            return true;
        }

        if (m_bounds.contains(x, y)) {
            if (track->getState() == TrackState::Idle) track->setState(TrackState::Playing);
            else track->setState(TrackState::Idle);
            return true;
        }
        return false;
    }

private:
    std::shared_ptr<FluxTrackNode> m_trackNode;
    float m_rotation = 0.0f;
};

} // namespace Beam

#endif // TAPE_REEL_HPP
