#ifndef TIMELINE_HPP
#define TIMELINE_HPP

#include "interface/core/component.hpp"
#include "interface/core/theme.hpp"
#include "engine/session/flux_project.hpp"
#include "engine/core/audio_engine.hpp"
#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace Beam {

enum class TimelineTool { Pointer, Scissors, Glue };

class Timeline : public Component {
public:
    Timeline(std::shared_ptr<FluxProject> project, AudioEngine* engine) 
        : m_project(project), m_engine(engine) {
        setName("Timeline");
        setVisible(false);
        m_zoom = 1.0f;
    }

    void setTool(TimelineTool tool) { m_tool = tool; }

    void sliceRegion(TrackData& track, size_t index, size_t offsetInFrames) {
        if (index >= track.regions.size()) return;
        auto original = track.regions[index];
        
        if (offsetInFrames <= 0 || offsetInFrames >= original.duration) return;

        // Split peaks for both halves
        std::vector<std::vector<float>> peaks1, peaks2;
        float splitRatio = (float)offsetInFrames / (float)original.duration;
        
        for (const auto& chan : original.channelPeaks) {
            size_t splitIdx = (size_t)(chan.size() * splitRatio);
            peaks1.push_back(std::vector<float>(chan.begin(), chan.begin() + splitIdx));
            peaks2.push_back(std::vector<float>(chan.begin() + splitIdx, chan.end()));
        }

        Region firstHalf = {original.name, original.startFrame, offsetInFrames, original.sourceOffset, original.trackIndex, peaks1};
        Region secondHalf = {original.name + " (Slice)", original.startFrame + offsetInFrames, original.duration - offsetInFrames, original.sourceOffset + offsetInFrames, original.trackIndex, peaks2};

        track.regions[index] = firstHalf;
        track.regions.insert(track.regions.begin() + index + 1, secondHalf);
        
        m_selectedRegionIndex = -1; // Deselect after slice
    }

    void paint(QuadBatcher& batcher) override {
        // Darkened Studio Background
        batcher.drawQuad(0, 0, m_bounds.w, m_bounds.h, 0.05f, 0.05f, 0.06f, 1.0f);

        float trackHeight = 100.0f;
        float pixelsPerSecond = 50.0f * m_zoom;
        float framesPerPixel = 44100.0f / pixelsPerSecond;

        // 1. Grid Lines
        float gridRes = (m_zoom > 2.0f) ? 0.1f : (m_zoom > 0.5f ? 1.0f : 5.0f);
        for (float s = 0; s < 10000; s += gridRes) {
            float sx = (s * pixelsPerSecond) - m_offsetX;
            if (sx < 0) continue;
            if (sx > m_bounds.w) break;
            batcher.drawQuad(sx, 30, 1, m_bounds.h - 30, 0.15f, 0.15f, 0.18f, 1.0f);
        }

        // 2. Lanes
        for (int i = 0; i < 20; ++i) {
            float y = 30 + (i * trackHeight) - m_offsetY;
            if (y + trackHeight < 30) continue;
            if (y > m_bounds.h) break;
            
            // Alternating Lane Colors
            Color laneCol = (i % 2 == 0) ? Color(0.12f, 0.12f, 0.14f) : Color(0.1f, 0.1f, 0.12f);
            batcher.drawQuad(0, y, m_bounds.w, trackHeight, laneCol.r, laneCol.g, laneCol.b, 1.0f);
            batcher.drawQuad(0, y + trackHeight - 1, m_bounds.w, 1, 0.2f, 0.2f, 0.25f, 0.6f);
        }

        // 3. Regions
        if (m_project) {
            for (auto& track : m_project->getTracks()) {
                for (size_t i = 0; i < track.regions.size(); ++i) {
                    auto& reg = track.regions[i];
                    float rx = (float)reg.startFrame / framesPerPixel - m_offsetX;
                    float ry = 30 + (track.trackIndex * trackHeight) + 5 - m_offsetY;
                    float rw = (float)reg.duration / framesPerPixel;
                    float rh = trackHeight - 10;

                    if (rx + rw < 0 || rx > m_bounds.w) continue;

                    bool isSelected = (m_selectedTrackPtr == &track && m_selectedRegionIndex == (int)i);
                    bool isHovered = (m_hoverTrackIdx == (int)track.trackIndex && m_hoverRegionIdx == (int)i);
                    
                    // Region Chassis
                    Color baseCol = isSelected ? Color(0.25f, 0.45f, 0.65f) : Color(0.18f, 0.2f, 0.22f);
                    if (isHovered && !m_isDraggingRegion) baseCol = baseCol.brighter(0.1f);
                    
                    batcher.drawBeveledRect(rx, ry, rw, rh, 3.0f, 0.5f, baseCol.r, baseCol.g, baseCol.b, 1.0f);
                    
                    // Tools Specific Highlighting
                    if (isHovered && !m_isDraggingRegion) {
                        if (m_tool == TimelineTool::Scissors) {
                             batcher.drawQuad(m_mouseX - 1, ry, 2, rh, 1.0f, 1.0f, 1.0f, 0.4f);
                        } else if (m_tool == TimelineTool::Glue) {
                             batcher.drawQuad(rx + rw - 4, ry, 4, rh, 0.2f, 1.0f, 0.4f, 0.6f);
                        }
                    }
                    
                    if (!reg.channelPeaks.empty()) {
                        float channelHeight = rh / (float)reg.channelPeaks.size();
                        for (size_t c = 0; c < reg.channelPeaks.size(); ++c) {
                            auto const& peaks = reg.channelPeaks[c];
                            if (peaks.empty()) continue;

                            float midY = ry + (c * channelHeight) + channelHeight * 0.5f;
                            float step = rw / (float)peaks.size();
                            
                            std::vector<std::pair<float, float>> top, bot;
                            for (size_t p = 0; p < peaks.size(); ++p) {
                                float px = rx + p * step;
                                float ph = peaks[p] * (channelHeight * 0.45f);
                                top.push_back({px, midY - ph});
                                bot.push_back({px, midY + ph});
                            }
                            
                            Color waveCol = isSelected ? Color(0.8f, 0.9f, 1.0f) : Theme::Emerald;
                            batcher.drawCurve(top, 1.2f, waveCol.r, waveCol.g, waveCol.b, 0.8f);
                            batcher.drawCurve(bot, 1.2f, waveCol.r, waveCol.g, waveCol.b, 0.8f);
                        }
                    }
                    
                    // Title Bar Overlay
                    batcher.drawQuad(rx, ry, rw, 15, 0.0f, 0.0f, 0.0f, 0.2f);
                    batcher.drawText(reg.name, rx + 6, ry + 2, 9, 0.9f, 0.9f, 0.9f, 1.0f);
                }
            }
        }

        // 4. Ruler (Hardware Style)
        batcher.drawChassisPanel(0, 0, m_bounds.w, 30, 0, 0.12f, 0.12f, 0.15f, 1.0f);
        for(float s=0; s < 10000; s += 1.0f) {
            float sx = (s * pixelsPerSecond) - m_offsetX;
            if (sx < 0) continue;
            if (sx > m_bounds.w) break;
            
            bool isMajor = (fmod(s, 5.0f) < 0.1f);
            batcher.drawQuad(sx, isMajor ? 5 : 15, 1, isMajor ? 25 : 15, 0.6f, 0.6f, 0.7f, 1.0f);
            if (isMajor) {
                char timeStr[16];
                int mins = (int)s / 60;
                int secs = (int)s % 60;
                snprintf(timeStr, 16, "%d:%02d", mins, secs);
                batcher.drawText(timeStr, sx + 4, 6, 9, 0.7f, 0.7f, 0.8f, 0.8f);
            }
        }

        // 5. Playhead & Cursors
        if (m_engine) {
            float playheadX = (float)m_engine->getCurrentFrame() / framesPerPixel - m_offsetX;
            if (playheadX >= 0 && playheadX <= m_bounds.w) {
                batcher.drawQuad(playheadX - 1, 0, 3, m_bounds.h, Theme::Red.r, Theme::Red.g, Theme::Red.b, 1.0f);
                batcher.drawQuad(playheadX - 6, 0, 12, 10, Theme::Red.r, Theme::Red.g, Theme::Red.b, 1.0f);
            }
        }
    }

    void handleKeyDown(int key) {
        if (!m_isVisible) return;
        if (key == 1073741904) m_offsetX = (std::max)(0.0f, m_offsetX - 50.0f);
        if (key == 1073741903) m_offsetX += 50.0f;
    }

    bool onMouseDown(float x, float y, int button, bool shift) override {
        if (!m_isVisible) return false;
        
        // Convert to local coordinates
        float lx = x - m_bounds.x;
        float ly = y - m_bounds.y;

        float pixelsPerSecond = 50.0f * m_zoom;
        float framesPerPixel = 44100.0f / pixelsPerSecond;

        if (button == 3) { m_isPanning = true; m_lastMouseX = lx; m_lastMouseY = ly; return true; }
        
        if (ly > 30) {
            float rx = lx + m_offsetX;
            size_t frame = (size_t)(rx * framesPerPixel);
            float trackHeight = 100.0f;
            int trackIdx = (int)((ly - 30 + m_offsetY) / trackHeight);
            
            if (trackIdx >= 0 && m_project && trackIdx < (int)m_project->getTracks().size()) {
                auto& track = m_project->getTracks()[trackIdx];
                for (size_t i = 0; i < track.regions.size(); ++i) {
                    auto& r = track.regions[i];
                    if (frame >= r.startFrame && frame < r.startFrame + r.duration) {
                        if (m_tool == TimelineTool::Scissors) {
                            sliceRegion(track, i, frame - r.startFrame);
                            return true;
                        } 
                        else if (m_tool == TimelineTool::Glue) {
                            if (i + 1 < track.regions.size()) {
                                auto& next = track.regions[i+1];
                                size_t gap = next.startFrame - (r.startFrame + r.duration);
                                if (gap < 44100) { 
                                    r.duration = (next.startFrame + next.duration) - r.startFrame;
                                    
                                    // Append peaks
                                    for (size_t c = 0; c < r.channelPeaks.size() && c < next.channelPeaks.size(); ++c) {
                                        r.channelPeaks[c].insert(r.channelPeaks[c].end(), next.channelPeaks[c].begin(), next.channelPeaks[c].end());
                                    }
                                    
                                    track.regions.erase(track.regions.begin() + i + 1);
                                    return true;
                                }
                            }
                        }
                        else if (m_tool == TimelineTool::Pointer) {
                            m_selectedTrackPtr = &track;
                            m_selectedRegionIndex = (int)i;
                            m_isDraggingRegion = true;
                            m_dragTrackIndex = trackIdx;
                            m_dragRegionIndex = (int)i;
                            m_dragOffsetFrame = frame - r.startFrame;
                            return true;
                        }
                    }
                }
                m_selectedTrackPtr = nullptr;
                m_selectedRegionIndex = -1;
                return true;
            }
        }

        if (ly < 30) {
            if (m_engine) m_engine->seek((size_t)((lx + m_offsetX) * framesPerPixel));
            return true;
        }
        return false;
    }

    bool onMouseMove(float x, float y, bool shift) override {
        if (!m_isVisible) return false;
        
        // Convert to local coordinates (subtract component origin)
        float lx = x - m_bounds.x;
        float ly = y - m_bounds.y;
        
        m_mouseX = lx; m_mouseY = ly;

        float pixelsPerSecond = 50.0f * m_zoom;
        float framesPerPixel = 44100.0f / pixelsPerSecond;
        float trackHeight = 100.0f;
        
        // Update Hover State
        m_hoverTrackIdx = -1; m_hoverRegionIdx = -1;
        if (ly > 30) {
             float rx = lx + m_offsetX;
             size_t frame = (size_t)(rx * framesPerPixel);
             int trackIdx = (int)((ly - 30 + m_offsetY) / trackHeight);
             if (trackIdx >= 0 && m_project && trackIdx < (int)m_project->getTracks().size()) {
                 auto& track = m_project->getTracks()[trackIdx];
                 for (int i=0; i<(int)track.regions.size(); ++i) {
                     if (frame >= track.regions[i].startFrame && frame < track.regions[i].startFrame + track.regions[i].duration) {
                         m_hoverTrackIdx = trackIdx;
                         m_hoverRegionIdx = i;
                         break;
                     }
                 }
             }
        }

        if (m_isDraggingRegion && m_selectedTrackPtr) {
            float rx = lx + m_offsetX;
            int64_t frame = (int64_t)(rx * framesPerPixel);
            int64_t newStart = frame - m_dragOffsetFrame;
            if (newStart < 0) newStart = 0;
            
            if (shift) newStart = (newStart / 44100) * 44100;
            
            if (m_dragRegionIndex >= 0 && m_dragRegionIndex < (int)m_selectedTrackPtr->regions.size()) {
                m_selectedTrackPtr->regions[m_dragRegionIndex].startFrame = (size_t)newStart;
                
                int newTrackIdx = (int)((ly - 30 + m_offsetY) / trackHeight);
                if (newTrackIdx != m_dragTrackIndex && newTrackIdx >= 0 && newTrackIdx < (int)m_project->getTracks().size()) {
                    auto& oldTrack = m_project->getTracks()[m_dragTrackIndex];
                    auto& newTrack = m_project->getTracks()[newTrackIdx];
                    
                    Region r = oldTrack.regions[m_dragRegionIndex];
                    r.trackIndex = newTrackIdx;
                    
                    oldTrack.regions.erase(oldTrack.regions.begin() + m_dragRegionIndex);
                    newTrack.regions.push_back(r);
                    
                    std::sort(newTrack.regions.begin(), newTrack.regions.end(), [](const Region& a, const Region& b) {
                        return a.startFrame < b.startFrame;
                    });
                    
                    m_selectedTrackPtr = &newTrack;
                    for (int i=0; i<(int)newTrack.regions.size(); ++i) {
                        if (newTrack.regions[i].startFrame == r.startFrame && newTrack.regions[i].name == r.name) {
                            m_selectedRegionIndex = i;
                            m_dragRegionIndex = i;
                            break;
                        }
                    }
                    m_dragTrackIndex = newTrackIdx;
                }
            }
            return true;
        }

        if (m_isPanning) {
            m_offsetX = (std::max)(0.0f, m_offsetX - (lx - m_lastMouseX));
            m_offsetY = (std::max)(0.0f, m_offsetY - (ly - m_lastMouseY));
            m_lastMouseX = lx; m_lastMouseY = ly; return true;
        }
        
        m_lastMouseX = lx; m_lastMouseY = ly;
        return false;
    }

    bool onMouseUp(float x, float y, int button, bool shift) override {
        m_isPanning = false; 
        m_isDraggingRegion = false;
        return true;
    }

    bool onMouseWheel(float x, float y, float delta, bool shift) override {
        if (!m_isVisible || !m_bounds.contains(x, y)) return false;
        
        float lx = x - m_bounds.x;
        float ly = y - m_bounds.y;

        float oldZoom = m_zoom;
        m_zoom *= (delta > 0) ? 1.1f : 0.9f;
        m_zoom = std::clamp(m_zoom, 0.01f, 100.0f);
        float localX = lx + m_offsetX;
        m_offsetX = (std::max)(0.0f, localX * (m_zoom / oldZoom) - lx);
        return true;
    }

private:
    std::shared_ptr<FluxProject> m_project;
    AudioEngine* m_engine;
    float m_offsetX = 0, m_offsetY = 0;
    float m_zoom = 1.0f;
    bool m_isPanning = false;
    float m_lastMouseX = 0, m_lastMouseY = 0;
    TrackData* m_selectedTrackPtr = nullptr;
    int m_selectedRegionIndex = -1;
    bool m_isDraggingRegion = false;
    int m_dragTrackIndex = -1;
    int m_dragRegionIndex = -1;
    int64_t m_dragOffsetFrame = 0;
    TimelineTool m_tool = TimelineTool::Pointer;
    
    // Hover / Cursor state
    float m_mouseX = 0, m_mouseY = 0;
    int m_hoverTrackIdx = -1;
    int m_hoverRegionIdx = -1;
};

} // namespace Beam

#endif