#ifndef WORKSPACE_HPP
#define WORKSPACE_HPP

#include "component.hpp"
#include "tape_reel.hpp"
#include "cable.hpp"
#include "filter_module.hpp"
#include "dynamics_module.hpp"
#include "../engine/flux_track_node.hpp"
#include "../engine/analog_suite.hpp"
#include "../engine/flux_fx_nodes.hpp"
#include "../engine/audio_engine.hpp"
#include "../session/flux_project.hpp"
#include "../utilities/flux_audio_utils.hpp"
#include <vector>
#include <iostream>
#include <cmath>

namespace Beam {

class Workspace : public Component {
public:
    Workspace(std::shared_ptr<FluxProject> project, AudioEngine* engine) 
        : m_project(project), m_engine(engine) {
        setBounds(0, 0, 10000, 10000); 
        m_zoom = 1.0f;
    }

    void update(float dt) override {
        syncReels();
        for (auto& module : m_modules) module->update(dt);
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        if (!m_isVisible) return;
        
        batcher.setViewTransform(m_panX, m_panY, m_zoom);

        // Background Grid (Virtual Space)
        float spacing = 50.0f;
        for (float x = 0; x < 5000; x += spacing) 
            batcher.drawQuad(x, 0, 1, 5000, 0.2f, 0.2f, 0.2f, 0.5f);
        for (float y = 0; y < 5000; y += spacing) 
            batcher.drawQuad(0, y, 5000, 1, 0.2f, 0.2f, 0.2f, 0.5f);

        for (auto& cable : m_cables) renderCable(batcher, cable, dt, screenH);

        for (auto& module : m_modules) module->render(batcher, dt, screenW, screenH);

        batcher.resetViewTransform(screenW, screenH);

        if (m_isDraggingCable && m_activePort) {
            float mx, my; SDL_GetMouseState(&mx, &my);
            // Transform mouse to virtual space
            float vmx = (mx - m_panX) / m_zoom;
            float vmy = (my - m_panY) / m_zoom;
            
            Rect pB = m_activePort->getBounds();
            float x1 = pB.x + pB.w / 2;
            float y1 = pB.y + pB.h / 2;

            float cx = (x1 + vmx) * 0.5f;
            float cy = (y1 + vmy) * 0.5f + std::abs(vmx - x1) * 0.2f + 20.0f;
            std::vector<std::pair<float, float>> curvePoints;
            for (int i = 0; i <= 16; ++i) {
                float t = (float)i / 16.0f;
                float invT = 1.0f - t;
                float px = invT * invT * x1 + 2.0f * invT * t * cx + t * t * vmx;
                float py = invT * invT * y1 + 2.0f * invT * t * cy + t * t * vmy;
                curvePoints.push_back({px, py});
            }
            batcher.setViewTransform(m_panX, m_panY, m_zoom);
            batcher.drawCurve(curvePoints, 3.0f, 1.0f, 0.8f, 0.2f, 0.6f);
            batcher.resetViewTransform(screenW, screenH);
        }

        if (m_isLoading) {
            m_loadingTimer += dt;
            float cx = screenW * 0.5f;
            float cy = screenH * 0.5f;
            for (int i = 0; i < 4; ++i) {
                float angle = m_loadingTimer * 5.0f + (i * 1.57f);
                float rx = cx + std::cos(angle) * 30.0f;
                float ry = cy + std::sin(angle) * 30.0f;
                batcher.drawRoundedRect(rx - 8, ry - 8, 16, 16, 8.0f, 0.5f, 0.2f, 0.6f, 1.0f, 1.0f);
            }
            batcher.drawText("PROCESSING TAPE...", cx - 60, cy + 60, 14, 1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    void syncReels() {
        if (!m_project) return;
        auto& tracks = m_project->getTracks();
        
        // Sync Tracks
        for (auto& track : tracks) {
            bool exists = false;
            for (auto& mod : m_modules) {
                auto reel = std::dynamic_pointer_cast<TapeReel>(mod);
                if (reel && reel->getNodeId() == track.nodeId) { exists = true; break; }
            }
            if (!exists) {
                float x = (400.0f + (track.trackIndex * 50.0f) + m_panX) / m_zoom;
                float y = (100.0f + (track.trackIndex * 150.0f) + m_panY) / m_zoom;
                auto reel = std::make_shared<TapeReel>(track.node, track.nodeId, x, y);
                setupModule(reel);
            }
        }

        // Sync Input Node
        auto nodes = m_project->getGraph()->getNodes();
        for (auto const& [id, node] : nodes) {
            if (node->getName() == "Audio Input" || node->getName() == "Master") {
                bool exists = false;
                for (auto& mod : m_modules) {
                    auto audioMod = std::dynamic_pointer_cast<AudioModule>(mod);
                    if (audioMod && audioMod->getNodeId() == id) { exists = true; break; }
                }
                if (!exists) {
                    float x = (node->getName() == "Master" ? 800.0f : 100.0f);
                    float y = (node->getName() == "Master" ? 250.0f : 100.0f);
                    auto mod = std::make_shared<AudioModule>(node, id, x, y);
                    setupModule(mod);
                }
            }
        }
    }

    void setupModule(std::shared_ptr<AudioModule> mod) {
        mod->onDeleteRequested = [this](AudioModule* m) { removeModule(m); };
        if (mod->getInputPort()) mod->getInputPort()->onConnectStarted = [this](Port* p) { startCableDrag(p); };
        if (mod->getOutputPort()) mod->getOutputPort()->onConnectStarted = [this](Port* p) { startCableDrag(p); };
        m_modules.push_back(mod);
    }

    void addTrack(const std::string& filePath, float x, float y, AudioEngine& engine) {
        m_isLoading = true;
        m_loadingTimer = 0.0f;
        
        std::string fileName = filePath;
        size_t lastSlash = filePath.find_last_of("/\\");
        if (lastSlash != std::string::npos) fileName = filePath.substr(lastSlash + 1);

        auto fluxTrack = std::make_shared<FluxTrackNode>(fileName, 1024 * 4);
        if (fluxTrack->load(filePath)) {
            size_t nodeId = m_project->getGraph()->addNode(fluxTrack);
            
            TrackData td;
            td.node = fluxTrack;
            td.nodeId = nodeId;
            td.trackIndex = (int)m_project->getTracks().size();
            
            size_t totalFrames = fluxTrack->getInternalNode()->getTotalFrames();
            Region r = {fileName, 0, totalFrames, 0, td.trackIndex};
            r.channelPeaks = fluxTrack->getPeakData(400); 
            td.regions.push_back(r);
            
            m_project->addTrack(td);
            syncReels(); 
        }
        m_isLoading = false;
    }

    void addFX(const std::string& type, float x, float y) {
        std::shared_ptr<FluxNode> fxNode;
        int buf = 1024 * 4;
        float sr = 44100.0f;

        x = (x - m_panX) / m_zoom;
        y = (y - m_panY) / m_zoom;

        if (type == "Filter") {
            auto node = std::make_shared<FluxFilterNode>(buf, sr);
            size_t id = m_project->getGraph()->addNode(node);
            setupModule(std::make_shared<FilterModule>(node, id, x, y));
            syncReels(); if (m_engine) m_engine->updatePlan();
            return;
        }

        if (type == "Opto-2A") {
            auto node = std::make_shared<Opto2A>(buf, sr);
            size_t id = m_project->getGraph()->addNode(node);
            setupModule(std::make_shared<DynamicsModule<Opto2A>>(node, id, x, y));
            syncReels(); if (m_engine) m_engine->updatePlan();
            return;
        }
        if (type == "FET-76") {
            auto node = std::make_shared<FET76>(buf, sr);
            size_t id = m_project->getGraph()->addNode(node);
            setupModule(std::make_shared<DynamicsModule<FET76>>(node, id, x, y));
            syncReels(); if (m_engine) m_engine->updatePlan();
            return;
        }
        if (type == "Tube Limiter") {
            auto node = std::make_shared<TubeLimiter>(buf, sr);
            size_t id = m_project->getGraph()->addNode(node);
            setupModule(std::make_shared<DynamicsModule<TubeLimiter>>(node, id, x, y));
            syncReels(); if (m_engine) m_engine->updatePlan();
            return;
        }
        if (type == "VCA-Bus") {
            auto node = std::make_shared<VCABus>(buf, sr);
            size_t id = m_project->getGraph()->addNode(node);
            setupModule(std::make_shared<DynamicsModule<VCABus>>(node, id, x, y));
            syncReels(); if (m_engine) m_engine->updatePlan();
            return;
        }
        if (type == "Vari-Mu") {
            auto node = std::make_shared<VariMu>(buf, sr);
            size_t id = m_project->getGraph()->addNode(node);
            setupModule(std::make_shared<DynamicsModule<VariMu>>(node, id, x, y));
            syncReels(); if (m_engine) m_engine->updatePlan();
            return;
        }

        if (type == "Tube-P EQ") fxNode = std::make_shared<TubeP_EQ>(buf, sr);
        else if (type == "Console-E") fxNode = std::make_shared<ConsoleE_EQ>(buf, sr);
        else if (type == "Vintage-G") fxNode = std::make_shared<VintageG_EQ>(buf, sr);
        else if (type == "Graphic-10") fxNode = std::make_shared<Graphic10_EQ>(buf, sr);
        else if (type == "Air-Lift") fxNode = std::make_shared<AirLift_EQ>(buf, sr);
        
        else if (type == "Steel Plate") fxNode = std::make_shared<SteelPlate>(buf, sr);
        else if (type == "Golden Hall") fxNode = std::make_shared<GoldenHall>(buf, sr);
        else if (type == "Copper Spring") fxNode = std::make_shared<CopperSpring>(buf, sr);
        else if (type == "Cathedral") fxNode = std::make_shared<Cathedral>(buf, sr);
        else if (type == "Grain Verb") fxNode = std::make_shared<GrainVerb>(buf, sr);

        else if (type == "Echo-Plex") fxNode = std::make_shared<EchoPlex>(buf, sr);
        else if (type == "BBD-Bucket") fxNode = std::make_shared<BBD_Bucket>(buf, sr);
        else if (type == "Reverse") fxNode = std::make_shared<Reverse_Delay>(buf, sr);
        else if (type == "Ping-Pong") fxNode = std::make_shared<PingPong_Delay>(buf, sr);
        else if (type == "Space Shift") fxNode = std::make_shared<SpaceShift>(buf, sr);

        else if (type == "Gain") fxNode = std::make_shared<FluxGainNode>(buf);
        else if (type == "Delay") fxNode = std::make_shared<FluxDelayNode>(buf, sr);
        else if (type == "Spectrum") fxNode = std::make_shared<FluxSpectrumAnalyzer>(buf, sr);
        else if (type == "Loudness") fxNode = std::make_shared<FluxLoudnessMeter>(buf, sr);
        else if (type == "Empty Tape") {
            auto fluxTrack = std::make_shared<FluxTrackNode>("Empty Tape", buf);
            size_t nodeId = m_project->getGraph()->addNode(fluxTrack);
            
            TrackData td;
            td.node = fluxTrack;
            td.nodeId = nodeId;
            td.trackIndex = (int)m_project->getTracks().size();
            
            m_project->addTrack(td);
            syncReels(); 
            return;
        }

        if (fxNode) {
            size_t id = m_project->getGraph()->addNode(fxNode);
            auto mod = std::make_shared<AudioModule>(fxNode, id, x, y);
            setupModule(mod);
            syncReels();
            if (m_engine) m_engine->updatePlan();
        }
    }

    void removeModule(AudioModule* mod) {
        size_t id = mod->getNodeId();
        m_project->getGraph()->removeNode(id);
        auto& tracks = m_project->getTracks();
        for(auto it = tracks.begin(); it != tracks.end(); ++it) {
            if(it->nodeId == id) { tracks.erase(it); break; }
        }
        for (auto it = m_cables.begin(); it != m_cables.end(); ) {
            if (it->input->getModule() == mod || it->output->getModule() == mod) it = m_cables.erase(it);
            else ++it;
        }
        for (auto it = m_modules.begin(); it != m_modules.end(); ++it) {
            if (it->get() == mod) { m_modules.erase(it); break; }
        }
        if (m_engine) m_engine->updatePlan();
    }

    void startCableDrag(Port* p) { 
        for (auto it = m_cables.begin(); it != m_cables.end(); ++it) {
            if (it->input == p || it->output == p) {
                m_project->getGraph()->disconnect(it->output->getModule()->getNodeId(), 0, it->input->getModule()->getNodeId(), 0);
                m_engine->updatePlan();
                m_activePort = (it->input == p) ? it->output : it->input;
                m_cables.erase(it);
                m_isDraggingCable = true;
                return;
            }
        }
        m_isDraggingCable = true; 
        m_activePort = p; 
    }

    void connectPorts(Port* p1, Port* p2) {
        if (!p1 || !p2 || p1->getType() == p2->getType() || !m_engine) return;
        Port* out = (p1->getType() == PortType::Output) ? p1 : p2;
        Port* in = (p1->getType() == PortType::Input) ? p1 : p2;
        m_cables.push_back({out, in});
        m_project->getGraph()->connect(out->getModule()->getNodeId(), 0, in->getModule()->getNodeId(), 0);
        m_engine->updatePlan();
    }

    bool onMouseDown(float x, float y, int button) override {
        if (!m_isVisible) return false;
        
        float vmx = (x - m_panX) / m_zoom;
        float vmy = (y - m_panY) / m_zoom;

        for (auto it = m_modules.rbegin(); it != m_modules.rend(); ++it) {
            if ((*it)->getBounds().contains(vmx, vmy)) return (*it)->onMouseDown(vmx, vmy, button);
        }
        if (button == 3) { m_isPanning = true; m_lastMouseX = x; m_lastMouseY = y; return true; }
        return false;
    }

    bool onMouseUp(float x, float y, int button) override {
        if (m_isDraggingCable) {
            float vmx = (x - m_panX) / m_zoom;
            float vmy = (y - m_panY) / m_zoom;

            for (auto& mod : m_modules) {
                auto audioMod = std::dynamic_pointer_cast<AudioModule>(mod);
                if (audioMod) {
                    auto checkPort = [&](std::shared_ptr<Port> p) {
                        if (!p) return false;
                        Rect b = p->getBounds();
                        float padding = 15.0f / m_zoom; // Scale padding with zoom
                        return (vmx >= b.x - padding && vmx <= b.x + b.w + padding && vmy >= b.y - padding && vmy <= b.y + b.h + padding);
                    };

                    if (checkPort(audioMod->getInputPort())) connectPorts(m_activePort, audioMod->getInputPort().get());
                    else if (checkPort(audioMod->getOutputPort())) connectPorts(m_activePort, audioMod->getOutputPort().get());
                }
            }
            m_isDraggingCable = false; m_activePort = nullptr;
            return true;
        }
        if (!m_isVisible) return false;
        m_isPanning = false;
        for (auto& mod : m_modules) mod->onMouseUp(x, y, button);
        return true;
    }

    bool onMouseMove(float x, float y) override {
        if (m_isDraggingCable) return true; 
        if (!m_isVisible) return false;
        
        float vmx = (x - m_panX) / m_zoom;
        float vmy = (y - m_panY) / m_zoom;

        if (m_isPanning) {
            m_panX += (x - m_lastMouseX); 
            m_panY += (y - m_lastMouseY);
            m_lastMouseX = x; m_lastMouseY = y; return true;
        }
        for (auto& mod : m_modules) if (mod->onMouseMove(vmx, vmy)) return true;
        return false;
    }

    bool onMouseWheel(float x, float y, float delta) override {
        if (!m_isVisible) return false;
        float oldZoom = m_zoom;
        float factor = (delta > 0) ? 1.1f : 0.9f;
        m_zoom *= factor;
        m_zoom = (std::clamp)(m_zoom, 0.1f, 5.0f);
        m_panX = x - (x - m_panX) * (m_zoom / oldZoom);
        m_panY = y - (y - m_panY) * (m_zoom / oldZoom);
        return true;
    }

    void setVisible(bool visible) { m_isVisible = visible; }

private:
    void renderCable(QuadBatcher& batcher, Cable& cable, float dt, float screenH) {
        Rect outPos = cable.output->getBounds();
        Rect inPos = cable.input->getBounds();
        float x1 = outPos.x + outPos.w / 2;
        float y1 = outPos.y + outPos.h / 2;
        float x2 = inPos.x + inPos.w / 2;
        float y2 = inPos.y + inPos.h / 2;
        float cx = (x1 + x2) * 0.5f;
        float cy = (y1 + y2) * 0.5f + std::abs(x2 - x1) * 0.2f + 20.0f * m_zoom;
        std::vector<std::pair<float, float>> curvePoints;
        for (int i = 0; i <= 24; ++i) {
            float t = (float)i / 24.0f;
            float invT = 1.0f - t;
            float px = invT * invT * x1 + 2.0f * invT * t * cx + t * t * x2;
            float py = invT * invT * y1 + 2.0f * invT * t * cy + t * t * y2;
            curvePoints.push_back({px, py});
        }
        batcher.drawCurve(curvePoints, 6.0f * m_zoom, 0.05f, 0.05f, 0.05f, 0.4f);
        batcher.drawCurve(curvePoints, 3.0f * m_zoom, 0.25f, 0.55f, 1.0f, 1.0f);
        batcher.drawCurve(curvePoints, 1.0f * m_zoom, 0.5f, 0.8f, 1.0f, 0.7f);
    }

    std::shared_ptr<FluxProject> m_project;
    AudioEngine* m_engine;
    std::vector<std::shared_ptr<AudioModule>> m_modules;
    std::vector<Cable> m_cables;
    float m_panX = 0, m_panY = 0;
    float m_zoom = 1.0f;
    bool m_isPanning = false;
    float m_lastMouseX = 0, m_lastMouseY = 0;
    bool m_isDraggingCable = false;
    Port* m_activePort = nullptr;
    bool m_isLoading = false;
    float m_loadingTimer = 0.0f;
};

} // namespace Beam

#endif // WORKSPACE_HPP