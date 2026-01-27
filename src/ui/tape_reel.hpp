#ifndef TAPE_REEL_HPP
#define TAPE_REEL_HPP

#include "audio_module.hpp"
#include "../dsp/track_node.hpp"

namespace Beam {

class TapeReel : public AudioModule {
public:
    TapeReel(const std::string& name, float x, float y, std::shared_ptr<TrackNode> track) 
        : AudioModule(name, x, y), m_track(track) {
        setBounds(x, y, 200, 120);
    }

    void render(QuadBatcher& batcher) override {
        // Base Module
        AudioModule::render(batcher);

        // Reel Graphics (Two circles/quads)
        float reelSize = 40.0f;
        float reelY = m_bounds.y + 40;
        
        // Spinning reel effect if playing
        float color = (m_track->getState() == TrackState::Playing) ? 0.6f : 0.4f;
        batcher.drawQuad(m_bounds.x + 30, reelY, reelSize, reelSize, color, color, color, 1.0f);
        batcher.drawQuad(m_bounds.x + 130, reelY, reelSize, reelSize, color, color, color, 1.0f);

        // Status Indicator
        if (m_track->getState() == TrackState::Recording) {
            batcher.drawQuad(m_bounds.x + 10, m_bounds.y + 10, 10, 10, 1.0f, 0.0f, 0.0f, 1.0f); // Red REC
        }
    }

    bool onMouseDown(float x, float y, int button) override {
        if (AudioModule::onMouseDown(x, y, button)) return true;

        // Toggle playback on click
        if (m_bounds.contains(x, y)) {
            if (m_track->getState() == TrackState::Idle) {
                m_track->setState(TrackState::Playing);
            } else {
                m_track->setState(TrackState::Idle);
            }
            return true;
        }
        return false;
    }

private:
    std::shared_ptr<TrackNode> m_track;
};

} // namespace Beam

#endif // TAPE_REEL_HPP
