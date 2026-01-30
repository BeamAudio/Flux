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
        setName("Timeline");
        setVisible(false);
        m_zoom = 1.0f;
    }

    void setTool(TimelineTool tool) { m_tool = tool; }

    void sliceRegion(TrackData& track, size_t index, size_t offsetInFrames) {
        if (index >= track.regions.size()) return;
        auto original = track.regions[index];
        
        if (offsetInFrames <= 0 || offsetInFrames >= original.duration) return;

        // Split peaks if possible, or just copy (simplified)
        std::vector<std::vector<float>> peaks1 = original.channelPeaks; // Naive
        std::vector<std::vector<float>> peaks2 = original.channelPeaks; // Naive

        Region firstHalf = {original.name, original.startFrame, offsetInFrames, original.sourceOffset, original.trackIndex, peaks1};
        Region secondHalf = {original.name + " (Slice)", original.startFrame + offsetInFrames, original.duration - offsetInFrames, original.sourceOffset + offsetInFrames, original.trackIndex, peaks2};

        track.regions[index] = firstHalf;
        track.regions.insert(track.regions.begin() + index + 1, secondHalf);
    }

    void renderCable(QuadBatcher& batcher, Cable& cable, float dt, float screenH) {
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 0.05f, 0.05f, 0.06f, 1.0f);

        float trackHeight = 100.0f;
        float pixelsPerSecond = 50.0f * m_zoom;
        float framesPerPixel = 44100.0f / pixelsPerSecond;

        // Ruler
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, 30, 0.12f, 0.12f, 0.14f, 1.0f);
        
        // Ticks
        for(float s=0; s < 10000; s += 1.0f) {
            float sx = m_bounds.x + (s * pixelsPerSecond) - m_offsetX;
            if (sx < m_bounds.x) continue;
            if (sx > m_bounds.x + m_bounds.w) break;
            batcher.drawQuad(sx, m_bounds.y + 15, 1, 15, 0.5f, 0.5f, 0.5f, 1.0f);
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

                    if (rx + rw < m_bounds.x || rx > m_bounds.x + m_bounds.w) continue;

                    bool isSelected = (m_selectedTrackPtr == &track && m_selectedRegionIndex == (int)i);
                    float emerald[3] = {0.13f, 0.62f, 0.42f};
                    batcher.drawRoundedRect(rx, ry, rw, rh, 4.0f, 0.5f, isSelected ? 0.3f : 0.15f, isSelected ? 0.7f : 0.25f, isSelected ? 0.5f : 0.2f, 1.0f);
                    
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
                            batcher.drawCurve(top, 1.2f, emerald[0], emerald[1], emerald[2], 1.0f);
                            batcher.drawCurve(bot, 1.2f, emerald[0], emerald[1], emerald[2], 1.0f);
                        }
                    }
                    batcher.drawText(reg.name, rx + 5, ry + 5, 10, 1.0f, 1.0f, 1.0f, 1.0f);
                }
            }
        }

        // Playhead
        if (m_engine) {
            float playheadX = m_bounds.x + (float)m_engine->getCurrentFrame() / framesPerPixel - m_offsetX;
            if (playheadX >= m_bounds.x && playheadX <= m_bounds.x + m_bounds.w) {
                batcher.drawQuad(playheadX - 1, m_bounds.y, 3, m_bounds.h, 0.56f, 0.03f, 0.03f, 1.0f); // BRAND_RED
            }
        }
    }

    void handleKeyDown(int key) {
        if (!m_isVisible) return;
        if (key == 1073741904) m_offsetX = (std::max)(0.0f, m_offsetX - 50.0f);
        if (key == 1073741903) m_offsetX += 50.0f;
    }

    bool onMouseDown(float x, float y, int button) override {
        if (!m_isVisible || !m_bounds.contains(x, y)) return false;
        float pixelsPerSecond = 50.0f * m_zoom;
        float framesPerPixel = 44100.0f / pixelsPerSecond;

        if (button == 3) { m_isPanning = true; m_lastMouseX = x; m_lastMouseY = y; return true; }
        
        // Tool Logic
        if (y > m_bounds.y + 30) { // Click on track area
            float rx = x - m_bounds.x + m_offsetX;
            size_t frame = (size_t)(rx * framesPerPixel);
            
            if (m_tool == TimelineTool::Scissors) {
                // Find region under mouse
                float trackHeight = 100.0f;
                int trackIdx = (int)((y - m_bounds.y - 30 + m_offsetY) / trackHeight);
                if (trackIdx >= 0 && m_project && trackIdx < (int)m_project->getTracks().size()) {
                    auto& track = m_project->getTracks()[trackIdx];
                    for (size_t i = 0; i < track.regions.size(); ++i) {
                        auto& r = track.regions[i];
                        if (frame >= r.startFrame && frame < r.startFrame + r.duration) {
                            sliceRegion(track, i, frame - r.startFrame);
                            return true;
                        }
                    }
                }
            } else if (m_tool == TimelineTool::Glue) {
                // Simplistic Glue: Merge current region with next if touching
                float trackHeight = 100.0f;
                int trackIdx = (int)((y - m_bounds.y - 30 + m_offsetY) / trackHeight);
                if (trackIdx >= 0 && m_project && trackIdx < (int)m_project->getTracks().size()) {
                    auto& track = m_project->getTracks()[trackIdx];
                    for (size_t i = 0; i < track.regions.size(); ++i) {
                        auto& r = track.regions[i];
                        if (frame >= r.startFrame && frame < r.startFrame + r.duration) {
                            // Found region, try to glue with next
                            if (i + 1 < track.regions.size()) {
                                auto& next = track.regions[i+1];
                                // Simple merge: extend duration
                                r.duration += next.duration;
                                track.regions.erase(track.regions.begin() + i + 1);
                                return true;
                            }
                        }
                    }
                }
            }
        }

        if (y < m_bounds.y + 30) {
            if (m_engine) m_engine->seek((size_t)((x - m_bounds.x + m_offsetX) * framesPerPixel));
            return true;
        }
        return false;
    }

    bool onMouseMove(float x, float y) override {
        if (!m_isVisible) return false;
        if (m_isPanning) {
            m_offsetX = (std::max)(0.0f, m_offsetX - (x - m_lastMouseX));
            m_offsetY = (std::max)(0.0f, m_offsetY - (y - m_lastMouseY));
            m_lastMouseX = x; m_lastMouseY = y; return true;
        }
        return false;
    }

    bool onMouseUp(float x, float y, int button) override {
        m_isPanning = false; return true;
    }

    bool onMouseWheel(float x, float y, float delta) override {
        if (!m_isVisible || !m_bounds.contains(x, y)) return false;
        float oldZoom = m_zoom;
        m_zoom *= (delta > 0) ? 1.1f : 0.9f;
        m_zoom = std::clamp(m_zoom, 0.01f, 100.0f);
        float localX = x - m_bounds.x + m_offsetX;
        m_offsetX = (std::max)(0.0f, localX * (m_zoom / oldZoom) - (x - m_bounds.x));
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
    TimelineTool m_tool = TimelineTool::Pointer;
};

} // namespace Beam

#endif