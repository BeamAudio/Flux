#ifndef TIMELINE_VIEW_HPP
#define TIMELINE_VIEW_HPP

#include "interface/core/component.hpp"
#include "interface/core/theme.hpp"
#include "engine/session/flux_project.hpp"
#include "engine/core/audio_engine.hpp"
#include "engine/session/automation.hpp"
#include <vector>
#include <iostream>
#include "interface/widgets/combo_box.hpp" // For PopupMenu
#include <SDL3/SDL.h>
#include <algorithm>
#include <iostream>

namespace Beam {

enum class TimelineTool { Pointer, Scissors, Glue };

class Timeline : public Component, public PopupHost {
public:
    Timeline(std::shared_ptr<FluxProject> project, AudioEngine* engine) 
        : m_project(project), m_engine(engine) {
        setName("Timeline");
        setVisible(false);
        m_zoom = 1.0f;
    }
    
    // PopupHost Implementation
    void showPopup(std::shared_ptr<Component> popup) override {
        m_popup = popup;
        if (m_popup) addChildComponent(m_popup);
    }

    void closePopup() override {
        if (m_popup) {
            removeChildComponent(m_popup.get());
            m_popup = nullptr;
        }
    }

    // Override render to handle popup overlay
    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        if (!m_isVisible) return;
        
        // Render existing content (paint() and children)
        // Since popup is now a child, Component::render will handle it correctly in local space!
        Component::render(batcher, dt, screenW, screenH); 
    }
    
    // Helper to find all available parameters in the signal chain
    std::vector<std::shared_ptr<Parameter>> findAllParameters(TrackData& track) {
        std::vector<std::shared_ptr<Parameter>> results;
        if (!m_project || !m_project->getGraph()) return results;
        
        auto graph = m_project->getGraph();
        std::vector<size_t> queue = { track.nodeId };
        std::set<size_t> visited;
        
        while (!queue.empty()) {
            size_t u = queue.front();
            queue.erase(queue.begin());
            
            if (visited.count(u)) continue;
            visited.insert(u);
            
            auto node = graph->getNode(u);
            if (!node) continue;
            
            // Collect Parameters
            for (auto& param : node->getParameterOrder()) {
                // Check if already automated?
                bool alreadyAutomated = false;
                for (auto& lane : track.automationLanes) {
                    if (lane->getParameter() == param) { alreadyAutomated = true; break; }
                }
                if (!alreadyAutomated) {
                    results.push_back(param);
                }
            }
            
            // Find Downstream
            for (const auto& conn : graph->getConnections()) {
                if (conn.srcNodeId == u) {
                    queue.push_back(conn.dstNodeId);
                }
            }
        }
        return results;
    }

    void setTool(TimelineTool tool) { m_tool = tool; }
    void setShowAutomation(bool show) { m_showAutomation = show; }
    bool isShowingAutomation() const { return m_showAutomation; }

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
        
        if (track.node) {
            track.node->setRegions(track.regions);
            if (m_engine) m_engine->updatePlan();
        }

        m_selectedRegionIndex = -1; // Deselect after slice
    }

    void paint(QuadBatcher& batcher) override {
        // Darkened Studio Background
        batcher.drawQuad(0, 0, m_bounds.w, m_bounds.h, 0.05f, 0.05f, 0.06f, 1.0f);

        float currentY = 30 - m_offsetY;
        float pixelsPerSecond = 50.0f * m_zoom;
        float framesPerPixel = 44100.0f / pixelsPerSecond;
        float mainTrackHeight = 100.0f;
        float laneHeight = 60.0f;

        // 1. Grid Lines
        float gridRes = (m_zoom > 2.0f) ? 0.1f : (m_zoom > 0.5f ? 1.0f : 5.0f);
        for (float s = 0; s < 10000; s += gridRes) {
            float sx = (s * pixelsPerSecond) - m_offsetX;
            if (sx < 0) continue;
            if (sx > m_bounds.w) break;
            batcher.drawQuad(sx, 30, 1, m_bounds.h - 30, 0.15f, 0.15f, 0.18f, 1.0f);
        }

        if (!m_project) return;

        // 2. Tracks & Lanes
        for (auto& track : m_project->getTracks()) {
            float trackStartY = currentY;
            
            // --- Main Track Area ---
            if (currentY + mainTrackHeight > 30 && currentY < m_bounds.h) {
                 // Track Background
                 Color trackBg = (track.trackIndex % 2 == 0) ? Color(0.12f, 0.12f, 0.14f) : Color(0.1f, 0.1f, 0.12f);
                 batcher.drawQuad(0, currentY, m_bounds.w, mainTrackHeight, trackBg.r, trackBg.g, trackBg.b, 1.0f);
                 batcher.drawQuad(0, currentY + mainTrackHeight - 1, m_bounds.w, 1, 0.2f, 0.2f, 0.25f, 0.6f);
            }
            
            // Render Regions
            for (size_t i = 0; i < track.regions.size(); ++i) {
                auto& reg = track.regions[i];
                float rx = (float)reg.startFrame / framesPerPixel - m_offsetX;
                float ry = currentY + 5; 
                float rw = (float)reg.duration / framesPerPixel;
                float rh = mainTrackHeight - 10;

                if (rx + rw < 0 || rx > m_bounds.w) continue;
                if (currentY > m_bounds.h || currentY + mainTrackHeight < 0) continue;

                bool isSelected = (m_selectedTrackPtr == &track && m_selectedRegionIndex == (int)i);
                bool isHovered = (m_hoverTrackIdx == (int)track.trackIndex && m_hoverRegionIdx == (int)i);
                
                // Region Chassis
                Color baseCol = isSelected ? Color(0.25f, 0.45f, 0.65f) : Color(0.18f, 0.2f, 0.22f);
                if (isHovered && !m_isDraggingRegion) baseCol = baseCol.brighter(0.1f);
                
                batcher.drawBeveledRect(rx, ry, rw, rh, 3.0f, 0.5f, baseCol.r, baseCol.g, baseCol.b, 1.0f);
                
                // ... (Tools Highlighting & Waveforms - same logic, just using ry) ...
                if (isHovered && !m_isDraggingRegion) {
                    if (m_tool == TimelineTool::Scissors) batcher.drawQuad(m_mouseX - 1, ry, 2, rh, 1.0f, 1.0f, 1.0f, 0.4f);
                    else if (m_tool == TimelineTool::Glue) batcher.drawQuad(rx + rw - 4, ry, 4, rh, 0.2f, 1.0f, 0.4f, 0.6f);
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
                
                batcher.drawQuad(rx, ry, rw, 15, 0.0f, 0.0f, 0.0f, 0.2f);
                batcher.drawVectorText(reg.name, rx + 6, ry + 2, 9, 0.9f, 0.9f, 0.9f, 1.0f);
            }

            currentY += mainTrackHeight;

            // --- Automation Lanes ---
            if (m_showAutomation) {
                for (size_t l = 0; l < track.automationLanes.size(); ++l) {
                    auto lane = track.automationLanes[l];
                    
                    if (currentY + laneHeight > 30 && currentY < m_bounds.h) {
                        // Lane Background (Darker/Indented feel)
                        batcher.drawQuad(0, currentY, m_bounds.w, laneHeight, 0.08f, 0.08f, 0.1f, 1.0f);
                        batcher.drawQuad(0, currentY + laneHeight - 1, m_bounds.w, 1, 0.15f, 0.15f, 0.18f, 1.0f);
                        
                        // Lane Header/Label
                        std::string paramName = "Unknown";
                        if(auto p = lane->getParameter()) paramName = p->getName();
                        batcher.drawVectorText(paramName, 10, currentY + 5, 10, 0.6f, 0.6f, 0.65f, 1.0f);
                        
                        // --- Arm/Override/Clear Buttons ---
                        float btnX = 150.0f;
                        float btnW = 22.0f;
                        float btnH = 16.0f;
                        float btnY = currentY + 3.0f;
                        
                        // Arm Button (Red circle when recording)
                        bool isRecording = lane->isRecording();
                        Color armColor = isRecording ? Color(0.9f, 0.3f, 0.3f) : Color(0.4f, 0.2f, 0.2f);
                        batcher.drawRoundedRect(btnX, btnY, btnW, btnH, 3.0f, 0.5f, armColor.r, armColor.g, armColor.b, 1.0f);
                        batcher.drawVectorText("R", btnX + 7, btnY + 2, 10, 1.f, 1.f, 1.f, isRecording ? 1.f : 0.5f);
                        btnX += btnW + 4;
                        
                        // Override Button (Yellow M when overriding)
                        bool isOverride = lane->isOverride();
                        Color ovrColor = isOverride ? Color(0.9f, 0.7f, 0.2f) : Color(0.35f, 0.35f, 0.2f);
                        batcher.drawRoundedRect(btnX, btnY, btnW, btnH, 3.0f, 0.5f, ovrColor.r, ovrColor.g, ovrColor.b, 1.0f);
                        batcher.drawVectorText("M", btnX + 6, btnY + 2, 10, 1.f, 1.f, 1.f, isOverride ? 1.f : 0.5f);
                        btnX += btnW + 4;
                        
                        // Clear Button (Gray X)
                        batcher.drawRoundedRect(btnX, btnY, btnW, btnH, 3.0f, 0.5f, 0.25f, 0.25f, 0.28f, 1.0f);
                        batcher.drawVectorText("X", btnX + 7, btnY + 2, 10, 0.9f, 0.4f, 0.4f, 1.0f);

                        // Points & Lines
                        auto& points = lane->getPoints();
                        if (!points.empty()) {
                            Color laneColor = (l == 0) ? Color(0.9f, 0.5f, 0.5f) : Color(0.5f, 0.9f, 0.5f);
                            std::vector<std::pair<float, float>> linePoints;
                            
                            // Pre-Point Extension
                            {
                                const auto& firstPt = points.front();
                                float px = (float)firstPt.frame / framesPerPixel - m_offsetX;
                                if (px > 0) {
                                    float normVal = 0.5f;
                                    if(auto p = lane->getParameter()) normVal = (firstPt.value - p->getMin()) / (p->getMax() - p->getMin());
                                    float py = currentY + laneHeight - (normVal * laneHeight);
                                    linePoints.push_back({0.0f, py});
                                    linePoints.push_back({px, py});
                                }
                            }

                            for (size_t ptIdx = 0; ptIdx < points.size(); ++ptIdx) {
                                const auto& pt = points[ptIdx];
                                float px = (float)pt.frame / framesPerPixel - m_offsetX;
                                float normVal = 0.5f;
                                if(auto p = lane->getParameter()) {
                                    normVal = (pt.value - p->getMin()) / (p->getMax() - p->getMin());
                                }
                                float py = currentY + laneHeight - (normVal * laneHeight);
                                
                                // Draw Segment to next point
                                if (ptIdx + 1 < points.size()) {
                                    const auto& nextPt = points[ptIdx + 1];
                                    float npx = (float)nextPt.frame / framesPerPixel - m_offsetX;
                                    float nNormVal = (nextPt.value - lane->getParameter()->getMin()) / (lane->getParameter()->getMax() - lane->getParameter()->getMin());
                                    float npy = currentY + laneHeight - (nNormVal * laneHeight);
                                    
                                    bool segHovered = (m_hoveredSegmentLaneIdx == (int)l && m_hoveredSegmentIdx == (int)ptIdx);
                                    Color segColor = segHovered ? Color(0.4f, 0.7f, 1.0f) : laneColor;

                                    if (pt.curvature == 0.0f) {
                                        batcher.drawLine(px, py, npx, npy, 2.0f, segColor.r, segColor.g, segColor.b, 0.8f);
                                    } else {
                                        // Draw curved segment using piecewise lines
                                        std::vector<std::pair<float, float>> curvePoints;
                                        int steps = 16;
                                        for (int s = 0; s <= steps; ++s) {
                                            float t = (float)s / (float)steps;
                                            float curvedT = t;
                                            if (pt.curvature > 0) curvedT = std::pow(t, 1.0f + pt.curvature * 4.0f);
                                            else curvedT = 1.0f - std::pow(1.0f - t, 1.0f - pt.curvature * 4.0f);
                                            
                                            float cx = px + t * (npx - px);
                                            float cy = py + curvedT * (npy - py);
                                            curvePoints.push_back({cx, cy});
                                        }
                                        batcher.drawCurve(curvePoints, 2.0f, segColor.r, segColor.g, segColor.b, 0.8f);
                                    }
                                }

                                // Point visual
                                bool isPtHovered = (m_hoveredPointLaneIdx == (int)l && m_hoveredPointIdx == (int)ptIdx);
                                bool isPtSelected = (m_selectedPointLaneIdx == (int)l && m_selectedPointIdx == (int)ptIdx);
                                float pointSize = (isPtHovered || isPtSelected) ? 8.0f : 6.0f;
                                Color ptColor = isPtSelected ? Color(0.3f, 0.9f, 0.9f) : (isPtHovered ? Color(1.f, 1.f, 1.f) : laneColor);
                                batcher.drawQuad(px - pointSize/2, py - pointSize/2, pointSize, pointSize, ptColor.r, ptColor.g, ptColor.b, 1.0f);
                            }
                        }
                    }
                    currentY += laneHeight;
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
                batcher.drawVectorText(timeStr, sx + 4, 6, 9, 0.7f, 0.7f, 0.8f, 0.8f);
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
        if (key == 1073741904) m_offsetX = (std::max)(0.0f, m_offsetX - 50.0f); // Left Arrow
        if (key == 1073741903) m_offsetX += 50.0f;                              // Right Arrow
        if (key == 97) setShowAutomation(!m_showAutomation);                    // 'A' to toggle Automation
    }

    bool onMouseDown(float x, float y, int button, bool shift) override {
        if (!m_isVisible) return false;
        
        // Convert to local coordinates
        float lx = x - m_bounds.x;
        float ly = y - m_bounds.y;

        if (m_popup) {
            Rect pb = m_popup->getBounds();
            if (pb.contains(lx, ly)) {
                return m_popup->onMouseDown(lx - pb.x, ly - pb.y, button, shift);
            } else {
                closePopup();
                return true; 
            }
        }

        float pixelsPerSecond = 50.0f * m_zoom;
        float framesPerPixel = 44100.0f / pixelsPerSecond;

        if (button == 3) { m_isPanning = true; m_lastMouseX = lx; m_lastMouseY = ly; return true; }
        
        if (ly > 30) {
            float rx = lx + m_offsetX;
            size_t frame = (size_t)(rx * framesPerPixel);
            float currentY = 30 - m_offsetY;
            float mainTrackHeight = 100.0f;
            float laneHeight = 60.0f;
            
            if (m_project) {
                for (size_t tIdx = 0; tIdx < m_project->getTracks().size(); ++tIdx) {
                    auto& track = m_project->getTracks()[tIdx];
                    int trackIdx = (int)tIdx;
                    float trackStartY = currentY;
                    float trackEndY = trackStartY + mainTrackHeight; 
                    
                    if (ly >= trackStartY && ly < trackEndY) {

                // ALT + Click: Create Volume Automation Lane (Testing)
                // 1073742050 is usually Alt key modifier in some SDL contexts, or check bool
                // But we don't have 'alt' bool, only 'shift'. 
                // Let's use Right Click (button == 3) purely for this test if panning wasn't consuming it?
                // Panning consumes button 3. 
                // Let's use Middle Click (button == 2) or just Ctrl+Shift (if we had ctrl).
                // I'll check 'shift' is true. If Shift+RightClick?
                // Panning is button 3. 
                // Let's use Double Click? No double click event.
                
                // Let's just use a specific area? Title bar of track.
                bool clickedTitle = (lx + m_offsetX < 100); // Rough check
                
                // Hack: If Shift+Click on Track Header (area with no regions usually? or just checking collision).
                // Actually, let's just use the fact that if we didn't hit a region/point, we can do track ops.
                
                // Check Automation Hit
                bool hitAutomationInfo = false;
                if (m_showAutomation && !track.automationLanes.empty()) {
                     // ... existing hit test ...
                }
                
                // 'A' Key + Click = Open Parameter Menu
                const bool *keys = SDL_GetKeyboardState(NULL);
                bool aHeld = keys[SDL_SCANCODE_A]; // Scancode for A
                
                if (aHeld) {
                     auto params = findAllParameters(track);
                     if (!params.empty()) {
                        std::vector<std::string> names;
                        for(auto p : params) names.push_back(p->getName()); 
                        
                        auto menu = std::make_shared<PopupMenu>(names, [this, &track, params](int index) {
                            if (index >= 0 && index < (int)params.size()) {
                                auto p = params[index];
                                auto lane = std::make_shared<AutomationLane>(p);
                                // Add initial point at start with current value so it's visible
                                lane->addPoint(0, p->getValue());
                                
                                track.automationLanes.push_back(lane);
                                if (m_engine) m_engine->addAutomationLane(lane);
                                // Ensure automation view is on
                                setShowAutomation(true);
                            }
                            closePopup();
                        });
                        
                        // Position menu at click location
                        float mh = (std::min)((float)names.size() * 20.0f + 10.0f, 400.0f);
                        menu->setBounds(lx, ly, 200, mh);
                        showPopup(menu);
                        return true;
                     }
                }
                
                // If we didn't hit automation point or region, and Shift is held:
                // Shift+Click now just adds a point if on a lane (handled below), 
                // BUT we are in the empty space of the track header/body here.
                // Previous logic created a lane via iterative discovery.
                // We REMOVE that since 'A'+Click replaces it.
                // If Shift is held here, do nothing? Or maybe just select track.



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
            
            currentY += mainTrackHeight;

            // Automation Lanes Hit Test
            if (m_showAutomation) {
                for (size_t l = 0; l < track.automationLanes.size(); ++l) {
                     float laneStart = currentY;
                     float laneEnd = laneStart + laneHeight;
                     
                     if (ly >= laneStart && ly < laneEnd) {
                         auto lane = track.automationLanes[l];
                         
                         // Button Hit Test (positioned at x=150, 176, 202)
                         float btnX = 150.0f;
                         float btnW = 22.0f;
                         float btnH = 16.0f;
                         float btnY = laneStart + 3.0f;
                         
                         // Arm Button
                         if (lx >= btnX && lx < btnX + btnW && ly >= btnY && ly < btnY + btnH) {
                             lane->setRecording(!lane->isRecording());
                             return true;
                         }
                         btnX += btnW + 4;
                         
                         // Override Button
                         if (lx >= btnX && lx < btnX + btnW && ly >= btnY && ly < btnY + btnH) {
                             lane->setOverride(!lane->isOverride());
                             return true;
                         }
                         btnX += btnW + 4;
                         
                         // Clear Button
                         if (lx >= btnX && lx < btnX + btnW && ly >= btnY && ly < btnY + btnH) {
                             lane->clear();
                             return true;
                         }
                         
                         // Point Hit Test - check if clicking on existing point
                         auto& points = lane->getPoints();
                         for (size_t ptIdx = 0; ptIdx < points.size(); ++ptIdx) {
                             // ... existing point hit test ...
                             const auto& pt = points[ptIdx];
                             float px = (float)pt.frame / framesPerPixel - m_offsetX;
                             float normVal = 0.5f;
                             if (auto p = lane->getParameter()) {
                                 normVal = (pt.value - p->getMin()) / (p->getMax() - p->getMin());
                             }
                             float py = laneStart + laneHeight - (normVal * laneHeight);
                             
                             if (std::abs(lx - px) < 8 && std::abs(ly - py) < 8) {
                                 m_selectedPointLaneIdx = (int)l;
                                 m_selectedPointIdx = (int)ptIdx;
                                 m_isDraggingPoint = true;
                                 m_dragPointTrackPtr = &track;
                                 m_dragPointLane = lane;
                                 m_dragPointLaneY = laneStart;
                                 return true;
                             }
                         }

                         // Segment Hit Test (Curvature) - If Ctrl held
                         const bool *keys = SDL_GetKeyboardState(NULL);
                         if (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL]) {
                             for (size_t ptIdx = 0; ptIdx + 1 < points.size(); ++ptIdx) {
                                 const auto& p1 = points[ptIdx];
                                 const auto& p2 = points[ptIdx+1];
                                 float x1 = (float)p1.frame / framesPerPixel - m_offsetX;
                                 float x2 = (float)p2.frame / framesPerPixel - m_offsetX;
                                 if (lx >= x1 && lx <= x2) {
                                     m_isDraggingCurvature = true;
                                     m_hoveredSegmentIdx = (int)ptIdx;
                                     m_hoveredSegmentLaneIdx = (int)l;
                                     m_dragPointLane = lane;
                                     m_dragStartCurvature = p1.curvature;
                                     m_lastMouseY = ly;
                                     return true;
                                 }
                             }
                         }
                         
                         // If Shift+Click and no point hit, add new point
                         if (shift) {
                             float localLaneY = ly - laneStart;
                             float val01 = 1.0f - (localLaneY / laneHeight);
                             val01 = std::clamp(val01, 0.0f, 1.0f);
                             if (auto p = lane->getParameter()) {
                                 float realVal = p->getMin() + val01 * (p->getMax() - p->getMin());
                                 lane->addPoint(frame, realVal);
                                 return true;
                             }
                         }
                         return true;
                     }
                     currentY += laneHeight;
                }
            }
                } // End Track Loop
            } // End Project Check
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
        
        if (m_popup) {
            Rect pb = m_popup->getBounds();
            if (pb.contains(lx, ly)) {
                return m_popup->onMouseMove(lx - pb.x, ly - pb.y, shift);
            }
        }
        
        m_mouseX = lx; m_mouseY = ly;

        float pixelsPerSecond = 50.0f * m_zoom;
        float framesPerPixel = 44100.0f / pixelsPerSecond;
        float trackHeight = 100.0f;
        
        // Update Hover State
        m_hoverTrackIdx = -1; m_hoverRegionIdx = -1;
        m_hoveredPointLaneIdx = -1; m_hoveredPointIdx = -1;
        m_hoveredSegmentIdx = -1; m_hoveredSegmentLaneIdx = -1;

        const bool *keys = SDL_GetKeyboardState(NULL);
        bool ctrlHeld = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];

        if (ly > 30) {
             float rx = lx + m_offsetX;
             size_t frame = (size_t)(rx * framesPerPixel);
             
             float currentY = 30 - m_offsetY;
             float mainTrackHeight = 100.0f;
             float laneHeight = 60.0f;
             
             if (m_project) {
                 for (size_t tIdx = 0; tIdx < m_project->getTracks().size(); ++tIdx) {
                     auto& track = m_project->getTracks()[tIdx];
                     float trackStartY = currentY;
                     float trackEndY = trackStartY + mainTrackHeight;
                     
                     if (ly >= trackStartY && ly < trackEndY) {
                         // ... existing region hover ...
                         for (int i=0; i<(int)track.regions.size(); ++i) {
                             if (frame >= track.regions[i].startFrame && frame < track.regions[i].startFrame + track.regions[i].duration) {
                                 m_hoverTrackIdx = (int)tIdx;
                                 m_hoverRegionIdx = i;
                                 break;
                             }
                         }
                     }
                     
                     currentY += mainTrackHeight;
                     
                     if (m_showAutomation) {
                         for (size_t l = 0; l < track.automationLanes.size(); ++l) {
                             float laneStart = currentY;
                             float laneEnd = laneStart + laneHeight;
                             if (ly >= laneStart && ly < laneEnd) {
                                 auto lane = track.automationLanes[l];
                                 auto& points = lane->getPoints();
                                 for (size_t ptIdx = 0; ptIdx < points.size(); ++ptIdx) {
                                     const auto& pt = points[ptIdx];
                                     float px = (float)pt.frame / framesPerPixel - m_offsetX;
                                     float normVal = (pt.value - lane->getParameter()->getMin()) / (lane->getParameter()->getMax() - lane->getParameter()->getMin());
                                     float py = laneStart + laneHeight - (normVal * laneHeight);
                                     if (std::abs(lx - px) < 8 && std::abs(ly - py) < 8) {
                                         m_hoveredPointLaneIdx = (int)l;
                                         m_hoveredPointIdx = (int)ptIdx;
                                         break;
                                     }
                                     if (ctrlHeld && ptIdx + 1 < points.size()) {
                                         float x1 = px;
                                         float x2 = (float)points[ptIdx+1].frame / framesPerPixel - m_offsetX;
                                         if (lx >= x1 && lx <= x2) {
                                             m_hoveredSegmentIdx = (int)ptIdx;
                                             m_hoveredSegmentLaneIdx = (int)l;
                                         }
                                     }
                                 }
                             }
                             currentY += laneHeight;
                         }
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
                    
                    // Sync both tracks
                    if (oldTrack.node) oldTrack.node->setRegions(oldTrack.regions);
                    if (newTrack.node) newTrack.node->setRegions(newTrack.regions);
                    if (m_engine) m_engine->updatePlan();

                    m_selectedTrackPtr = &newTrack;
                    for (int i=0; i<(int)newTrack.regions.size(); ++i) {
                        if (newTrack.regions[i].startFrame == r.startFrame && newTrack.regions[i].name == r.name) {
                            m_selectedRegionIndex = i;
                            m_dragRegionIndex = i;
                            break;
                        }
                    }
                    m_dragTrackIndex = newTrackIdx;
                } else {
                    // Just moved within same track
                    if (m_selectedTrackPtr->node) m_selectedTrackPtr->node->setRegions(m_selectedTrackPtr->regions);
                    // Don't need to rebuild plan for just position change within same node usually,
                    // but since processor stores a COPY of regions, we MUST rebuild or update processor.
                    if (m_engine) m_engine->updatePlan();
                }
            }
            return true;
        }
        
        // Point Dragging
        if (m_isDraggingPoint && m_dragPointLane) {
            float pixelsPerSecond = 50.0f * m_zoom;
            float framesPerPixel = 44100.0f / pixelsPerSecond;
            float laneHeight = 60.0f;
            
            // Calculate new frame
            float rx = lx + m_offsetX;
            size_t newFrame = (size_t)(std::max)(0.0f, rx * framesPerPixel);
            
            // Calculate value based on Y position within the lane
            if (auto param = m_dragPointLane->getParameter()) {
                float localLaneY = ly - m_dragPointLaneY;
                float val01 = 1.0f - (localLaneY / laneHeight);
                val01 = std::clamp(val01, 0.0f, 1.0f);
                float realVal = param->getMin() + val01 * (param->getMax() - param->getMin());
                
                m_dragPointLane->updatePoint(m_selectedPointIdx, newFrame, realVal);
            }
            return true;
        }

        if (m_isDraggingCurvature && m_dragPointLane) {
            float deltaY = m_lastMouseY - ly;
            float sens = 0.01f;
            float newCurv = m_dragStartCurvature + deltaY * sens;
            m_dragPointLane->setCurvature(m_hoveredSegmentIdx, newCurv);
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
        // Local coordinates for popup check (calculated in mouse down/move usually, but needed here)
        float lx = x - m_bounds.x;
        float ly = y - m_bounds.y;
        
        if (m_popup) {
            Rect pb = m_popup->getBounds();
            if (pb.contains(lx, ly)) {
                 return m_popup->onMouseUp(lx - pb.x, ly - pb.y, button, shift);
            }
        }

        m_isPanning = false; 
        m_isDraggingRegion = false;
        m_isDraggingPoint = false;
        m_isDraggingCurvature = false;
        m_dragPointLane = nullptr;
        m_dragPointTrackPtr = nullptr;
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
    bool m_showAutomation = false;
    
    // Automation point state
    int m_hoveredPointLaneIdx = -1;
    int m_hoveredPointIdx = -1;
    int m_selectedPointLaneIdx = -1;
    int m_selectedPointIdx = -1;
    bool m_isDraggingPoint = false;
    int m_hoveredSegmentIdx = -1;
    int m_hoveredSegmentLaneIdx = -1;
    bool m_isDraggingCurvature = false;
    float m_dragStartCurvature = 0.0f;
    TrackData* m_dragPointTrackPtr = nullptr;
    std::shared_ptr<AutomationLane> m_dragPointLane;
    float m_dragPointLaneY = 0.0f;  // Y position of the lane being dragged
    
    std::shared_ptr<Component> m_popup;
};

} // namespace Beam

#endif // TIMELINE_VIEW_HPP