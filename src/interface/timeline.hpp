#ifndef TIMELINE_HPP
#define TIMELINE_HPP

#include "component.hpp"
#include "../session/flux_project.hpp"
#include "../engine/audio_engine.hpp"
#include <vector>
#include <iostream>
#include <cmath>

namespace Beam {

enum class TimelineTool { Pointer, Scissors, Glue };

class Timeline : public Component {
public:
    Timeline(std::shared_ptr<FluxProject> project, AudioEngine* engine) 
        : m_project(project), m_engine(engine) {
        m_isVisible = false;
        m_zoom = 1.0f;
    }

    void setTool(TimelineTool tool) { m_tool = tool; }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        if (!m_isVisible) return;

        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 0.05f, 0.05f, 0.06f, 1.0f);

        float trackHeight = 100.0f;
        float pixelsPerSecond = 50.0f * m_zoom;
        float framesPerPixel = 44100.0f / pixelsPerSecond;

        // Ruler
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, 30, 0.12f, 0.12f, 0.14f, 1.0f);
        
        // Dynamic Ticks
        float targetSpacing = 100.0f; 
        float minInterval = targetSpacing / pixelsPerSecond;
        float magnitude = std::pow(10.0f, std::floor(std::log10(minInterval)));
        float residual = minInterval / magnitude;
        float interval;
        if (residual > 5.0f) interval = 10.0f * magnitude;
        else if (residual > 2.0f) interval = 5.0f * magnitude;
        else if (residual > 1.0f) interval = 2.0f * magnitude;
        else interval = magnitude;

        for(float s=0; s < 10000; s += interval) {
            float sx = m_bounds.x + (s * pixelsPerSecond) - m_offsetX;
            if (sx < m_bounds.x) continue;
            if (sx > m_bounds.x + m_bounds.w) break;
            batcher.drawQuad(sx, m_bounds.y + 15, 1, 15, 0.5f, 0.5f, 0.5f, 1.0f);
            char timeStr[16]; 
            if (interval < 1.0f) snprintf(timeStr, 16, "%.2fs", s);
            else snprintf(timeStr, 16, "%.0fs", s);
            batcher.drawText(timeStr, sx + 4, m_bounds.y + 5, 10, 0.7f, 0.7f, 0.7f, 1.0f);
        }

        // Lanes
        for (int i = 0; i < 20; ++i) {
            float y = m_bounds.y + 30 + (i * trackHeight) - m_offsetY;
            if (y + trackHeight < m_bounds.y + 30) continue;
            if (y > m_bounds.y + m_bounds.h) break;
            batcher.drawQuad(m_bounds.x, y + trackHeight - 1, m_bounds.w, 1, 0.15f, 0.15f, 0.15f, 1.0f);
        }

        // Regions
        if (m_project) {
            for (auto& track : m_project->getTracks()) {
                for (size_t i = 0; i < track.regions.size(); ++i) {
                    auto& reg = track.regions[i];
                    float rx = m_bounds.x + (float)reg.startFrame / framesPerPixel - m_offsetX;
                    float ry = m_bounds.y + 30 + (track.trackIndex * trackHeight) + 5 - m_offsetY;
                    float rw = (float)reg.duration / framesPerPixel;
                    float rh = trackHeight - 10;

                    if (rx + rw < m_bounds.x || rx > m_bounds.x + m_bounds.w || 
                        ry + rh < m_bounds.y + 30 || ry > m_bounds.y + m_bounds.h) continue;

                    bool isSelected = (m_selectedTrackPtr == &track && m_selectedRegionIndex == (int)i);
                    float rCol = isSelected ? 0.3f : 0.2f;
                    float gCol = isSelected ? 0.45f : 0.35f;
                    float bCol = isSelected ? 0.7f : 0.5f;

                    batcher.drawRoundedRect(rx, ry, rw, rh, 4.0f, 0.5f, rCol, gCol, bCol, 1.0f);
                    
                    if (!reg.channelPeaks.empty()) {
                        float channelHeight = rh / (float)reg.channelPeaks.size();
                        for (size_t c = 0; c < reg.channelPeaks.size(); ++c) {
                            auto const& peaks = reg.channelPeaks[c];
                            float midY = ry + (c * channelHeight) + channelHeight * 0.5f;
                            float step = rw / (float)peaks.size();
                            std::vector<std::pair<float, float>> top, bot;
                            for (size_t p = 0; p < peaks.size(); ++p) {
                                float px = rx + p * step;
                                float ph = peaks[p] * (channelHeight * 0.45f);
                                top.push_back({px, midY - ph});
                                bot.push_back({px, midY + ph});
                            }
                            batcher.drawCurve(top, 1.2f, 0.6f, 0.9f, 1.0f, 1.0f);
                            batcher.drawCurve(bot, 1.2f, 0.6f, 0.9f, 1.0f, 1.0f);
                        }
                    }
                    batcher.drawText(reg.name, rx + 5, ry + 5, 10, 0.9f, 0.9f, 0.9f, 1.0f);
                }
            }
        }

        // Playhead
        if (m_engine) {
            float playheadX = m_bounds.x + (float)m_engine->getCurrentFrame() / framesPerPixel - m_offsetX;
            if (playheadX >= m_bounds.x && playheadX <= m_bounds.x + m_bounds.w) {
                batcher.drawQuad(playheadX - 1, m_bounds.y, 3, m_bounds.h, 1.0f, 0.3f, 0.3f, 1.0f);
            }
        }

        // Tool Visuals
        if (m_isVisible) {
            float mx, my; SDL_GetMouseState(&mx, &my);
            if (m_bounds.contains(mx, my)) {
                if (m_tool == TimelineTool::Scissors) {
                    batcher.drawQuad(mx, m_bounds.y + 30, 1, m_bounds.h - 30, 1.0f, 1.0f, 1.0f, 0.5f);
                    batcher.drawText("CUT", mx + 5, my - 15, 10, 1.0f, 1.0f, 1.0f, 1.0f);
                } else if (m_tool == TimelineTool::Glue) {
                    batcher.drawText("GLUE", mx + 5, my - 15, 10, 1.0f, 1.0f, 1.0f, 1.0f);
                }
            }
        }
    }

    void handleKeyDown(int key) {
        if (!m_isVisible) return;
        if (m_selectedTrackPtr && m_selectedRegionIndex != -1) {
            float framesPerPixel = 44100.0f / (50.0f * m_zoom);
            size_t nudge = (size_t)(framesPerPixel * 10.0f);
            if (key == 1073741904) { // LEFT
                if (m_selectedTrackPtr->regions[m_selectedRegionIndex].startFrame > nudge)
                    m_selectedTrackPtr->regions[m_selectedRegionIndex].startFrame -= nudge;
                else m_selectedTrackPtr->regions[m_selectedRegionIndex].startFrame = 0;
            } else if (key == 1073741903) { // RIGHT
                m_selectedTrackPtr->regions[m_selectedRegionIndex].startFrame += nudge;
            }
        } else {
            if (key == 1073741904) m_offsetX = (std::max)(0.0f, m_offsetX - 50.0f);
            if (key == 1073741903) m_offsetX += 50.0f;
        }
    }

    bool onMouseDown(float x, float y, int button) override {
        if (!m_isVisible) return false;
        float pixelsPerSecond = 50.0f * m_zoom;
        float framesPerPixel = 44100.0f / pixelsPerSecond;

        if (button == 3) { m_isPanning = true; m_lastMouseX = x; m_lastMouseY = y; return true; }
        if (y < m_bounds.y + 30) {
            m_isScrubbing = true;
            m_engine->seek((size_t)((x - m_bounds.x + m_offsetX) * framesPerPixel));
            return true;
        }

        float trackHeight = 100.0f;
        if (m_project) {
            for (auto& track : m_project->getTracks()) {
                for (size_t i=0; i < track.regions.size(); ++i) {
                    auto& reg = track.regions[i];
                    float rx = m_bounds.x + (float)reg.startFrame / framesPerPixel - m_offsetX;
                    float ry = m_bounds.y + 30 + (track.trackIndex * trackHeight) + 5 - m_offsetY;
                    float rw = (float)reg.duration / framesPerPixel;
                    float rh = trackHeight - 10;

                    if (x >= rx && x <= rx + rw && y >= ry && y <= ry + rh) {
                        if (m_tool == TimelineTool::Scissors) {
                            sliceRegion(track, i, (size_t)((x - rx) * framesPerPixel));
                            return true;
                        } else if (m_tool == TimelineTool::Glue) {
                            glueRegions(track, i);
                            return true;
                        } else if (button == 1) {
                            m_isDraggingRegion = true; m_dragTrackPtr = &track; m_dragRegionIndex = (int)i; m_dragOffsetX = x - rx;
                            m_selectedTrackPtr = &track; m_selectedRegionIndex = (int)i;
                            return true;
                        }
                    }
                }
            }
        }
        m_selectedTrackPtr = nullptr; m_selectedRegionIndex = -1;
        return false;
    }

    // handleKeyDown, onMouseMove, onMouseUp, onMouseWheel, setVisible omitted for brevity...
    // But they must be kept! I will use a precise replace.
    // Actually, I'll just write the entire corrected file to be safe.


    bool onMouseMove(float x, float y) override {
        float pixelsPerSecond = 50.0f * m_zoom;
        float framesPerPixel = 44100.0f / pixelsPerSecond;

        if (m_isPanning) {
            m_offsetX = (std::max)(0.0f, m_offsetX - (x - m_lastMouseX));
            m_offsetY = (std::max)(0.0f, m_offsetY - (y - m_lastMouseY));
            m_lastMouseX = x; m_lastMouseY = y; return true;
        }
        if (m_isScrubbing) {
            m_engine->seek((size_t)((x - m_bounds.x + m_offsetX) * framesPerPixel));
            return true;
        }
        if (m_isDraggingRegion && m_dragTrackPtr) {
            float newX = x - m_dragOffsetX - m_bounds.x + m_offsetX;
            m_dragTrackPtr->regions[m_dragRegionIndex].startFrame = (size_t)((std::max)(0.0f, newX) * framesPerPixel);
            return true;
        }
        return false;
    }

    bool onMouseUp(float x, float y, int button) override {
        m_isDraggingRegion = false; m_isScrubbing = false; m_isPanning = false; m_dragTrackPtr = nullptr;
        return true;
    }

    bool onMouseWheel(float x, float y, float delta) override {
        if (!m_isVisible) return false;
        float oldZoom = m_zoom;
        m_zoom *= (delta > 0) ? 1.1f : 0.9f;
        m_zoom = std::clamp(m_zoom, 0.01f, 100.0f);
        float localX = x - m_bounds.x + m_offsetX;
        m_offsetX = (std::max)(0.0f, localX * (m_zoom / oldZoom) - (x - m_bounds.x));
        return true;
    }

    void setVisible(bool visible) { m_isVisible = visible; }

private:
    void sliceRegion(TrackData& track, size_t index, size_t offsetInFrames) {
        if (offsetInFrames <= 0 || offsetInFrames >= track.regions[index].duration) return;
        Region original = track.regions[index];
        track.regions[index].duration = offsetInFrames;
        
        float ratio = (float)offsetInFrames / original.duration;
        std::vector<std::vector<float>> p1, p2;
        for (auto const& ch : original.channelPeaks) {
            size_t split = (size_t)(ch.size() * ratio);
            p1.push_back(std::vector<float>(ch.begin(), ch.begin() + split));
            p2.push_back(std::vector<float>(ch.begin() + split, ch.end()));
        }
        track.regions[index].channelPeaks = p1;
        Region second = {original.name + " (Slice)", original.startFrame + offsetInFrames, original.duration - offsetInFrames, original.sourceOffset + offsetInFrames, original.trackIndex, p2};
        track.regions.insert(track.regions.begin() + index + 1, second);
    }

    void glueRegions(TrackData& track, size_t index) {
        if (index >= track.regions.size() - 1) return;
        Region& r1 = track.regions[index];
        Region& r2 = track.regions[index + 1];
        
        long gapFrames = (long)r2.startFrame - (long)(r1.startFrame + r1.duration);
        
        if (gapFrames > 0) {
            // Add silence gap
            r1.duration += gapFrames + r2.duration;
            for(size_t c=0; c < r1.channelPeaks.size(); ++c) {
                // Approximate silence in peaks (1 peak per pixel approx)
                size_t silenceCount = (size_t)((float)gapFrames / 44100.0f * 50.0f); // scale to visual
                r1.channelPeaks[c].insert(r1.channelPeaks[c].end(), silenceCount, 0.0f);
                if (c < r2.channelPeaks.size())
                    r1.channelPeaks[c].insert(r1.channelPeaks[c].end(), r2.channelPeaks[c].begin(), r2.channelPeaks[c].end());
            }
        } else {
            // Overlap or perfect touch
            size_t newEnd = (std::max)(r1.startFrame + r1.duration, r2.startFrame + r2.duration);
            r1.duration = newEnd - r1.startFrame;
            
            // Simple merge: append remaining part of r2 peaks
            for(size_t c=0; c < r1.channelPeaks.size(); ++c) {
                if (c < r2.channelPeaks.size())
                    r1.channelPeaks[c].insert(r1.channelPeaks[c].end(), r2.channelPeaks[c].begin(), r2.channelPeaks[c].end());
            }
        }
        
        track.regions.erase(track.regions.begin() + index + 1);
    }

    std::shared_ptr<FluxProject> m_project;
    AudioEngine* m_engine;
    bool m_isVisible = false;
    bool m_isDraggingRegion = false;
    bool m_isScrubbing = false;
    bool m_isPanning = false;
    float m_offsetX = 0, m_offsetY = 0;
    float m_zoom = 1.0f;
    TimelineTool m_tool = TimelineTool::Pointer;
    TrackData* m_dragTrackPtr = nullptr;
    int m_dragRegionIndex = -1;
    float m_dragOffsetX = 0;
    float m_lastMouseX = 0, m_lastMouseY = 0;
    TrackData* m_selectedTrackPtr = nullptr;
    int m_selectedRegionIndex = -1;
};

} // namespace Beam

#endif // TIMELINE_HPP





