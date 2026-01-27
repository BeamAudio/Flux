#ifndef TAPE_REEL_HPP
#define TAPE_REEL_HPP

#include "audio_module.hpp"
#include "../dsp/flux_track_node.hpp"

namespace Beam {

class TapeReel : public AudioModule {
public:
    TapeReel(std::shared_ptr<FluxTrackNode> track, float x, float y) 
        : AudioModule(track, x, y), m_trackNode(track) {
        setBounds(x, y, 200, 120);
    }

    void render(QuadBatcher& batcher) override {
        // Base Module
        AudioModule::render(batcher);

        auto track = m_trackNode->getInternalNode();
        // Reel Graphics (Two circles/quads)
        float reelSize = 40.0f;
        float reelY = m_bounds.y + 40;
        
        // Spinning reel effect if playing
        float color = (track->getState() == TrackState::Playing) ? 0.6f : 0.4f;
        batcher.drawQuad(m_bounds.x + 30, reelY, reelSize, reelSize, color, color, color, 1.0f);
        batcher.drawQuad(m_bounds.x + 130, reelY, reelSize, reelSize, color, color, color, 1.0f);

        // Status Indicator
        if (track->getState() == TrackState::Recording) {
            batcher.drawQuad(m_bounds.x + 10, m_bounds.y + 10, 10, 10, 1.0f, 0.0f, 0.0f, 1.0f); // Red REC
        }
    }

    bool onMouseDown(float x, float y, int button) override {
        if (AudioModule::onMouseDown(x, y, button)) return true;

        auto track = m_trackNode->getInternalNode();
        // Toggle playback on click
        if (m_bounds.contains(x, y)) {
            if (track->getState() == TrackState::Idle) {
                track->setState(TrackState::Playing);
            } else {
                track->setState(TrackState::Idle);
            }
            return true;
        }
        return false;
    }

private:
    std::shared_ptr<FluxTrackNode> m_trackNode;
};

} // namespace Beam

#endif // TAPE_REEL_HPP
