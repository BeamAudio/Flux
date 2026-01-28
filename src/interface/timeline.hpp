#ifndef TIMELINE_HPP
#define TIMELINE_HPP

#include "component.hpp"
#include "../session/flux_project.hpp"
#include "../engine/audio_engine.hpp"
#include <vector>
#include <iostream>
#include <cmath>

namespace Beam {

class Timeline : public Component {
public:
    Timeline(std::shared_ptr<FluxProject> project, AudioEngine* engine) 
        : m_project(project), m_engine(engine) {
        m_isVisible = false;
    }

    void render(QuadBatcher& batcher) override {
        if (!m_isVisible) return;

        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 0.05f, 0.05f, 0.06f, 1.0f);

        float trackHeight = 100.0f;
        float pixelsPerSecond = 50.0f;
        float framesPerPixel = 44100.0f / pixelsPerSecond;

        // Ruler
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, 30, 0.12f, 0.12f, 0.14f, 1.0f);
        for(float s=0; s < 1000; s += 1.0f) {
            float sx = m_bounds.x + (s * pixelsPerSecond);
            if (sx > m_bounds.x + m_bounds.w) break;
            batcher.drawQuad(sx, m_bounds.y + 15, 1, 15, 0.4f, 0.4f, 0.4f, 1.0f);
        }

        // Lanes
        for (int i = 0; i < 10; ++i) {
            float y = m_bounds.y + 30 + (i * trackHeight);
            batcher.drawQuad(m_bounds.x, y + trackHeight - 1, m_bounds.w, 1, 0.15f, 0.15f, 0.15f, 1.0f);
        }

        // Regions & Waveforms
        if (m_project) {
            for (auto& track : m_project->getTracks()) {
                for (auto& reg : track.regions) {
                    float rx = m_bounds.x + (float)reg.startFrame / framesPerPixel;
                    float ry = m_bounds.y + 30 + (track.trackIndex * trackHeight) + 5;
                    float rw = (float)reg.duration / framesPerPixel;
                    float rh = trackHeight - 10;

                    batcher.drawRoundedRect(rx, ry, rw, rh, 4.0f, 0.5f, 0.2f, 0.35f, 0.5f, 1.0f);
                    
                    if (!reg.peaks.empty()) {
                        std::vector<std::pair<float, float>> topPoints;
                        std::vector<std::pair<float, float>> bottomPoints;
                        float midY = ry + rh * 0.5f;
                        float step = rw / (float)reg.peaks.size();
                        for (size_t p = 0; p < reg.peaks.size(); ++p) {
                            float px = rx + p * step;
                            float ph = reg.peaks[p] * (rh * 0.45f);
                            topPoints.push_back({px, midY - ph});
                            bottomPoints.push_back({px, midY + ph});
                        }
                        batcher.drawCurve(topPoints, 1.5f, 0.5f, 0.8f, 1.0f, 1.0f);
                        batcher.drawCurve(bottomPoints, 1.5f, 0.5f, 0.8f, 1.0f, 1.0f);
                    }
                    batcher.drawText(reg.name, rx + 5, ry + 5, 10, 0.9f, 0.9f, 0.9f, 1.0f);
                }
            }
        }

        // Playhead (Drawn on top)
        if (m_engine) {
            float playheadX = m_bounds.x + (float)m_engine->getCurrentFrame() / framesPerPixel;
            if (playheadX >= m_bounds.x && playheadX <= m_bounds.x + m_bounds.w) {
                batcher.drawQuad(playheadX - 1, m_bounds.y, 3, m_bounds.h, 1.0f, 0.3f, 0.3f, 1.0f);
                batcher.drawRoundedRect(playheadX - 6, m_bounds.y, 12, 12, 6.0f, 0.5f, 1.0f, 0.3f, 0.3f, 1.0f);
            }
        }
    }

    bool onMouseDown(float x, float y, int button) override {
        if (!m_isVisible) return false;
        float pixelsPerSecond = 50.0f;
        float framesPerPixel = 44100.0f / pixelsPerSecond;

        // Scrubber / Seek interaction
        if (y < m_bounds.y + 30) {
            m_isScrubbing = true;
            size_t targetFrame = (size_t)((x - m_bounds.x) * framesPerPixel);
            if (m_engine) m_engine->seek(targetFrame);
            return true;
        }

        // Regions
        float trackHeight = 100.0f;
        if (m_project) {
            for (auto& track : m_project->getTracks()) {
                for (size_t i=0; i < track.regions.size(); ++i) {
                    auto& reg = track.regions[i];
                    float rx = m_bounds.x + (float)reg.startFrame / framesPerPixel;
                    float ry = m_bounds.y + 30 + (track.trackIndex * trackHeight) + 5;
                    float rw = (float)reg.duration / framesPerPixel;
                    float rh = trackHeight - 10;

                    if (x >= rx && x <= rx + rw && y >= ry && y <= ry + rh) {
                        if (button == 1) { 
                            m_isDraggingRegion = true; m_dragTrackPtr = &track; m_dragRegionIndex = (int)i; m_dragOffsetX = x - rx;
                            return true;
                        } else if (button == 3) { 
                            sliceRegion(track, i, (size_t)((x - rx) * framesPerPixel));
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    bool onMouseMove(float x, float y) override {
        float pixelsPerSecond = 50.0f;
        float framesPerPixel = 44100.0f / pixelsPerSecond;

        if (m_isScrubbing) {
            size_t targetFrame = (size_t)((x - m_bounds.x) * framesPerPixel);
            if (m_engine) m_engine->seek(targetFrame);
            return true;
        }

        if (m_isDraggingRegion && m_dragTrackPtr) {
            float newX = x - m_dragOffsetX - m_bounds.x;
            m_dragTrackPtr->regions[m_dragRegionIndex].startFrame = (size_t)((std::max)(0.0f, newX) * framesPerPixel);
            int newTrack = (int)((y - (m_bounds.y + 30)) / 100.0f);
            m_dragTrackPtr->regions[m_dragRegionIndex].trackIndex = std::clamp(newTrack, 0, 9);
            m_dragTrackPtr->trackIndex = std::clamp(newTrack, 0, 9); 
            return true;
        }
        return false;
    }

    bool onMouseUp(float x, float y, int button) override {
        m_isDraggingRegion = false;
        m_isScrubbing = false;
        m_dragTrackPtr = nullptr;
        return true;
    }

    void setVisible(bool visible) { m_isVisible = visible; }

private:
    void sliceRegion(TrackData& track, size_t index, size_t offsetInFrames) {
        if (offsetInFrames <= 0 || offsetInFrames >= track.regions[index].duration) return;
        Region original = track.regions[index];
        track.regions[index].duration = offsetInFrames;
        float ratio = (float)offsetInFrames / original.duration;
        size_t peakSplit = (size_t)(original.peaks.size() * ratio);
        std::vector<float> p1(original.peaks.begin(), original.peaks.begin() + peakSplit);
        std::vector<float> p2(original.peaks.begin() + peakSplit, original.peaks.end());
        track.regions[index].peaks = p1;
        Region secondHalf = {original.name + " (Slice)", original.startFrame + offsetInFrames, original.duration - offsetInFrames, original.sourceOffset + offsetInFrames, original.trackIndex, p2};
        track.regions.push_back(secondHalf);
    }

    std::shared_ptr<FluxProject> m_project;
    AudioEngine* m_engine;
    bool m_isVisible = false;
    bool m_isDraggingRegion = false;
    bool m_isScrubbing = false;
    TrackData* m_dragTrackPtr = nullptr;
    int m_dragRegionIndex = -1;
    float m_dragOffsetX = 0;
};

} // namespace Beam

#endif // TIMELINE_HPP
