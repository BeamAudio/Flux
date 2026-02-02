#ifndef WORKSPACE_HPP
#define WORKSPACE_HPP

#include "interface/core/component.hpp"
#include "interface/core/theme.hpp"
#include "interface/core/coordinate_system.hpp"
#include "interface/views/tape_reel.hpp"
#include "interface/modules/cable.hpp"
#include "interface/popup/popup_host.hpp"
#include "interface/modules/audio_module.hpp"
#include "engine/nodes/flux_track_node.hpp"
#include "engine/scripting/flux_script_node.hpp"
#include "engine/nodes/analog_suite.hpp"
#include "engine/nodes/flux_fx_nodes.hpp"
#include "engine/core/audio_engine.hpp"
#include "engine/core/audio_device_manager.hpp"
#include "engine/plugins/plugin_registry.hpp"
#include "engine/session/flux_project.hpp"
#include "engine/dsp/flux_audio_utils.hpp"
#include <vector>
#include <iostream>
#include <cmath>
#include <SDL3/SDL.h>

namespace Beam {

class Workspace : public Component, public PopupHost {
public:
    Workspace(std::shared_ptr<FluxProject> project, AudioEngine* engine, AudioDeviceManager* deviceManager = nullptr) 
        : m_project(project), m_engine(engine), m_deviceManager(deviceManager) {
        setName("Workspace");
        setBounds(0, 0, 10000, 10000); 
        m_zoom = 1.0f;
        setClipsChildren(false); 

        registerBuiltInPlugins();
    }

    void registerBuiltInPlugins() {
        auto& reg = PluginRegistry::get();
        // Dynamics
        reg.registerPlugin("Opto-2A", [](int b, float s){ return std::make_shared<Opto2A>(b, s); });
        reg.registerPlugin("FET-76", [](int b, float s){ return std::make_shared<FET76>(b, s); });
        reg.registerPlugin("Tube Limiter", [](int b, float s){ return std::make_shared<TubeLimiter>(b, s); });
        reg.registerPlugin("Lookahead Limiter", [](int b, float s){ return std::make_shared<LookaheadLimiter>(b, s); });
        reg.registerPlugin("VCA-Bus", [](int b, float s){ return std::make_shared<VCABus>(b, s); });
        reg.registerPlugin("Vari-Mu", [](int b, float s){ return std::make_shared<VariMu>(b, s); });
        
        // EQ
        reg.registerPlugin("Tube-P EQ", [](int b, float s){ return std::make_shared<TubeP_EQ>(b, s); });
        reg.registerPlugin("Console-E", [](int b, float s){ return std::make_shared<ConsoleE_EQ>(b, s); });
        reg.registerPlugin("Vintage-G", [](int b, float s){ return std::make_shared<VintageG_EQ>(b, s); });
        reg.registerPlugin("Graphic-10", [](int b, float s){ return std::make_shared<Graphic10_EQ>(b, s); });
        reg.registerPlugin("Air-Lift", [](int b, float s){ return std::make_shared<AirLift_EQ>(b, s); });

        // Reverb
        reg.registerPlugin("Steel Plate", [](int b, float s){ return std::make_shared<SteelPlate>(b, s); });
        reg.registerPlugin("Golden Hall", [](int b, float s){ return std::make_shared<GoldenHall>(b, s); });
        reg.registerPlugin("Copper Spring", [](int b, float s){ return std::make_shared<CopperSpring>(b, s); });
        reg.registerPlugin("Cathedral", [](int b, float s){ return std::make_shared<Cathedral>(b, s); });
        reg.registerPlugin("Grain Verb", [](int b, float s){ return std::make_shared<GrainVerb>(b, s); });

        // Delay
        reg.registerPlugin("Echo-Plex", [](int b, float s){ return std::make_shared<EchoPlex>(b, s); });
        reg.registerPlugin("BBD-Bucket", [](int b, float s){ return std::make_shared<BBD_Bucket>(b, s); });
        reg.registerPlugin("Reverse", [](int b, float s){ return std::make_shared<Reverse_Delay>(b, s); });
        reg.registerPlugin("Ping-Pong", [](int b, float s){ return std::make_shared<PingPong_Delay>(b, s); });
        reg.registerPlugin("Space Shift", [](int b, float s){ return std::make_shared<SpaceShift>(b, s); });

        // Utilities
        reg.registerPlugin("Spectrum", [](int b, float s){ return std::make_shared<FluxSpectrumAnalyzer>(b, s); });
        reg.registerPlugin("Loudness", [](int b, float s){ return std::make_shared<FluxLoudnessMeter>(b, s); });
        reg.registerPlugin("Gain", [](int b, float s){ return std::make_shared<FluxGainNode>(b); });
        reg.registerPlugin("Filter", [](int b, float s){ return std::make_shared<FluxFilterNode>(b, s); });
        reg.registerPlugin("Delay", [](int b, float s){ return std::make_shared<FluxDelayNode>(b, s); });
    }

    void localToScreen(float& x, float& y) override {
        CoordinateSystem::get().worldToScreen(x, y, x, y);
    }

    void screenToLocal(float& x, float& y) override {
        float wx, wy;
        CoordinateSystem::get().screenToWorld(x, y, wx, wy);
        x = wx;
        y = wy;
    }

    void showPopup(std::shared_ptr<Component> popup) override {
        m_popup = popup;
        if(m_popup) m_popup->setParent(this);
    }

    void closePopup() override {
        m_popup = nullptr;
    }

    void update(float dt) override {
        syncReels();
        Component::update(dt);
    }

    void paint(QuadBatcher& batcher) override {}

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        if (!m_isVisible) return; 
        
        auto& coords = CoordinateSystem::get();
        coords.setZoom(m_zoom);
        coords.setPan(m_panX, m_panY);
        coords.setWorkspaceOrigin(m_bounds.x, m_bounds.y);
        coords.setScreenDimensions(screenW, screenH);

        // 1. Global Coordinate System Setup
        batcher.pushViewTransform();
        // Set only Origin first so pushClip captures correct screen position
        batcher.setViewTransform(0, 0, 1.0f, m_bounds.x, m_bounds.y);

        batcher.pushOffset(0, 0); 
        batcher.pushClip(0, 0, m_bounds.w, m_bounds.h, screenH);
        
        // 2. Set actual workspace pan/zoom transform for content
        batcher.setViewTransform(m_panX, m_panY, m_zoom, m_bounds.x, m_bounds.y);

        // Background Grid (Truly infinite, view-relative)
        float spacing = 50.0f;
        float startX = std::floor(-m_panX / (spacing * m_zoom)) * spacing - spacing;
        float startY = std::floor(-m_panY / (spacing * m_zoom)) * spacing - spacing;
        float endX = startX + (screenW / m_zoom) + spacing * 2;
        float endY = startY + (screenH / m_zoom) + spacing * 2;
        
        for (float x = startX; x < endX; x += spacing) 
            batcher.drawQuad(x, startY, 1, endY - startY, 0.2f, 0.2f, 0.2f, 0.5f);
        for (float y = startY; y < endY; y += spacing) 
            batcher.drawQuad(startX, y, endX - startX, 1, 0.2f, 0.2f, 0.2f, 0.5f);

        for (auto& cable : m_cables) renderCable(batcher, cable, dt, screenH);
        
        for (auto& child : m_children) {
            child->render(batcher, dt, screenW, screenH);
        }

        if (m_popup) {
            batcher.popViewTransform(); 
            batcher.pushViewTransform();
            batcher.resetViewTransform(screenW, screenH);
            m_popup->render(batcher, dt, screenW, screenH);
            batcher.popViewTransform(); 
            batcher.pushViewTransform(); 
        }

        batcher.popViewTransform();
        batcher.popClip(screenH);
        batcher.popOffset();
        
        // Final safety reset for any components rendered after Workspace in the same frame
        batcher.resetViewTransform(screenW, screenH);

        if (m_isDraggingCable && m_activePort) {
            float mx, my;
            SDL_GetMouseState(&mx, &my);
            float localX = mx - m_bounds.x;
            float localY = my - m_bounds.y;
            float vmx = (localX - m_panX) / m_zoom;
            float vmy = (localY - m_panY) / m_zoom;
            
            Rect pB = m_activePort->getBounds();
            float modX = m_activePort->getParent()->getX();
            float modY = m_activePort->getParent()->getY();
            
            float x1 = modX + pB.x + pB.w / 2;
            float y1 = modY + pB.y + pB.h / 2;

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
            
            batcher.setViewTransform(m_panX, m_panY, m_zoom, m_bounds.x, m_bounds.y);
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
                batcher.drawRoundedRect(rx - 8, ry - 8, 16, 16, 8.0f, 0.5f, Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b, 1.0f);
            }
            batcher.drawText("PROCESSING TAPE...", cx - 60, cy + 60, 14, 1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    void syncReels() {
        if (!m_project) return;
        auto& tracks = m_project->getTracks();
        
        for (auto& track : tracks) {
            bool exists = false;
            for (auto& mod : m_modules) {
                auto reel = std::dynamic_pointer_cast<TapeReel>(mod);
                if (reel && reel->getNodeId() == track.nodeId) { exists = true; break; }
            }
            if (!exists) {
                float x = 400.0f + (track.trackIndex * 50.0f);
                float y = 100.0f + (track.trackIndex * 150.0f);
                auto reel = std::make_shared<TapeReel>(track.node, track.nodeId, x, y);
                setupModule(reel);
            }
        }

        auto nodes = m_project->getGraph()->getNodes();
        for (auto const& [id, node] : nodes) {
            bool exists = false;
            for (auto& mod : m_modules) {
                if (mod->getNodeId() == id) { exists = true; break; }
            }
            if (!exists) {
                float x = 100.0f;
                float y = 100.0f;
                if (node->getName() == "Master") { x = 800.0f; y = 250.0f; }
                
                auto mod = std::make_shared<AudioModule>(node, id, x, y, m_deviceManager);
                mod->setDraggable(true);
                setupModule(mod);
            }
        }
    }

    void setupModule(std::shared_ptr<AudioModule> mod) {
        mod->onDeleteRequested = [this](AudioModule* m) { removeModule(m); };
        if (mod->getInputPort()) mod->getInputPort()->onConnectStarted = [this](Port* p) { startCableDrag(p); };
        if (mod->getOutputPort()) mod->getOutputPort()->onConnectStarted = [this](Port* p) { startCableDrag(p); };
        if (mod->getSidechainPort()) mod->getSidechainPort()->onConnectStarted = [this](Port* p) { startCableDrag(p); };
        m_modules.push_back(mod);
        addChildComponent(mod);
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
        int buf = 1024 * 4;
        float sr = 44100.0f;

        if (type == "Empty Tape") {
            auto fluxTrack = std::make_shared<FluxTrackNode>("Empty Tape", buf);
            size_t nodeId = m_project->getGraph()->addNode(fluxTrack);
            TrackData td; td.node = fluxTrack; td.nodeId = nodeId; td.trackIndex = (int)m_project->getTracks().size();
            m_project->addTrack(td); 
        } else if (type == "Audio Input") {
            auto node = std::make_shared<InputNode>(buf);
            m_project->getGraph()->addNode(node);
        } else {
            auto node = PluginRegistry::get().createPlugin(type, buf, sr);
            if (node) {
                m_project->getGraph()->addNode(node);
            } else {
                std::cout << "Error: Unknown FX type '" << type << "'" << std::endl;
            }
        }

        syncReels();
        if (m_engine) m_engine->updatePlan();
    }

    void addScriptFX(const std::string& path, float x, float y) {
        float sr = 44100.0f;
        int buf = 1024 * 4;
        auto node = std::make_shared<FluxScriptNode>(path, buf, sr);
        size_t id = m_project->getGraph()->addNode(node);
        float vx = (x - m_panX) / m_zoom;
        float vy = (y - m_panY) / m_zoom;
        auto mod = std::make_shared<AudioModule>(node, id, vx, vy, m_deviceManager);
        setupModule(mod);
        syncReels();
        if (m_engine) m_engine->updatePlan();
    }

    void removeModule(AudioModule* mod) {
        if (!mod) return;
        
        if (m_activePort && m_activePort->getParent() == mod) {
            m_activePort = nullptr;
            m_isDraggingCable = false;
        }

        size_t id = mod->getNodeId();
        m_project->getGraph()->removeNode(id);
        
        auto& tracks = m_project->getTracks();
        for(auto it = tracks.begin(); it != tracks.end(); ++it) {
            if(it->nodeId == id) { tracks.erase(it); break; }
        }

        for (auto it = m_cables.begin(); it != m_cables.end(); ) {
            if (it->input->getParent() == mod || it->output->getParent() == mod) {
                it = m_cables.erase(it);
            } else {
                ++it;
            }
        }

        for (auto it = m_modules.begin(); it != m_modules.end(); ++it) {
            if (it->get() == mod) { 
                auto modPtr = *it;
                removeChildComponent(modPtr.get());
                m_modules.erase(it); 
                GarbageCollector::get().defer(modPtr);
                break; 
            }
        }
        
        if (m_engine) m_engine->updatePlan();
    }

    void startCableDrag(Port* p) { 
        for (auto it = m_cables.begin(); it != m_cables.end(); ++it) {
            if (it->input == p || it->output == p) {
                auto* outMod = dynamic_cast<AudioModule*>(it->output->getParent());
                auto* inMod = dynamic_cast<AudioModule*>(it->input->getParent());
                
                if (outMod && inMod) {
                    m_project->getGraph()->disconnect(outMod->getNodeId(), 0, inMod->getNodeId(), 0);
                    m_engine->updatePlan();
                }
                
                m_activePort = (it->input == p) ? it->output : it->input;
                m_cables.erase(it);
                m_isDraggingCable = true;
                return;
            }
        }
        m_isDraggingCable = true; m_activePort = p; 
    }

    void connectPorts(Port* p1, Port* p2, bool isSidechain = false) {
        if (!p1 || !p2 || p1->getType() == p2->getType() || !m_engine) return;
        Port* out = (p1->getType() == PortType::Output) ? p1 : p2;
        Port* in = (p1->getType() == PortType::Input || p1->getType() == PortType::Sidechain) ? p1 : p2;
        m_cables.push_back({out, in});
        
        auto* outMod = dynamic_cast<AudioModule*>(out->getParent());
        auto* inMod = dynamic_cast<AudioModule*>(in->getParent());

        int inPortIdx = isSidechain ? 1 : 0; 
        if(outMod && inMod)
            m_project->getGraph()->connect(outMod->getNodeId(), 0, inMod->getNodeId(), inPortIdx);
        m_engine->updatePlan();
    }

    bool onMouseDown(float x, float y, int button, bool shift) override {
        if (!m_isVisible || !m_bounds.contains(x, y)) return false;
        
        if (m_popup) {
            if (m_popup->getBounds().contains(x, y)) {
                return m_popup->onMouseDown(x - m_popup->getX(), y - m_popup->getY(), button, shift);
            } else {
                closePopup();
                return true; 
            }
        }

        float vmx, vmy;
        CoordinateSystem::get().screenToWorld(x, y, vmx, vmy);
        
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if ((*it)->onMouseDown(vmx, vmy, button, shift)) return true;
        }

        if (button == 3) { m_isPanning = true; m_lastMouseX = x; m_lastMouseY = y; return true; }
        
        mouseDown(MouseEvent(vmx, vmy, button, shift));
        return true;
    }

    bool onMouseUp(float x, float y, int button, bool shift) override {
        if (m_popup) {
            if (m_popup->getBounds().contains(x, y)) {
                m_popup->onMouseUp(x - m_popup->getX(), y - m_popup->getY(), button, shift);
            }
            return true; 
        }
        
        float vmx, vmy;
        CoordinateSystem::get().screenToWorld(x, y, vmx, vmy);

        if (m_isDraggingCable) {
            for (auto& mod : m_modules) {
                auto checkPort = [&](std::shared_ptr<Port> p) {
                    if (!p) return false;
                    Rect b = p->getBounds(); 
                    float padding = 15.0f / m_zoom;
                    float px = mod->getX() + b.x;
                    float py = mod->getY() + b.y;
                    return (vmx >= px - padding && vmx <= px + b.w + padding && vmy >= py - padding && vmy <= py + b.h + padding);
                };
                if (checkPort(mod->getInputPort())) connectPorts(m_activePort, mod->getInputPort().get());
                else if (checkPort(mod->getSidechainPort())) connectPorts(m_activePort, mod->getSidechainPort().get(), true);
                else if (checkPort(mod->getOutputPort())) connectPorts(m_activePort, mod->getOutputPort().get());
            }
            m_isDraggingCable = false; m_activePort = nullptr; return true;
        }
        m_isPanning = false;
        
        for (auto& child : m_children) child->onMouseUp(vmx, vmy, button, shift);
        mouseUp(MouseEvent(vmx, vmy, button, shift));
        return true;
    }

    bool onMouseMove(float x, float y, bool shift) override {
        if (!m_isVisible) return false;

        if (m_popup) {
            m_popup->onMouseMove(x - m_popup->getX(), y - m_popup->getY(), shift);
            return true;
        }

        if (m_isPanning) {
            m_panX += (x - m_lastMouseX); 
            m_panY += (y - m_lastMouseY);
            m_lastMouseX = x;
            m_lastMouseY = y; 
            CoordinateSystem::get().setPan(m_panX, m_panY);
            return true;
        }

        float vmx, vmy;
        CoordinateSystem::get().screenToWorld(x, y, vmx, vmy);

        if (m_isDraggingCable) return true; 
        
        for (auto& child : m_children) child->onMouseMove(vmx, vmy, shift);
        mouseMove(MouseEvent(vmx, vmy, 0, shift));
        
        m_lastMouseX = x;
        m_lastMouseY = y;
        return true;
    }

    bool onMouseWheel(float x, float y, float delta, bool shift) override {
        if (!m_isVisible || !m_bounds.contains(x, y)) return false;
        
        float oldZoom = m_zoom;
        m_zoom *= (delta > 0) ? 1.1f : 0.9f;
        m_zoom = (std::clamp)(m_zoom, 0.1f, 5.0f);
        
        float localX = x - m_bounds.x;
        float localY = y - m_bounds.y;

        m_panX = localX - (localX - m_panX) * (m_zoom / oldZoom);
        m_panY = localY - (localY - m_panY) * (m_zoom / oldZoom);
        
        CoordinateSystem::get().setZoom(m_zoom);
        CoordinateSystem::get().setPan(m_panX, m_panY);
        return true;
    }

private:
   void renderCable(QuadBatcher& batcher, Cable& cable, float dt, float screenH) {
        if (!cable.output || !cable.input) return;
        Rect outPos = cable.output->getBounds();
        Rect inPos = cable.input->getBounds();
        auto* outMod = dynamic_cast<AudioModule*>(cable.output->getParent());
        auto* inMod = dynamic_cast<AudioModule*>(cable.input->getParent());
        if (!outMod || !inMod) return;

        float x1 = outPos.x + outMod->getX() + outPos.w / 2;
        float y1 = outPos.y + outMod->getY() + outPos.h / 2;
        float x2 = inPos.x + inMod->getX() + inPos.w / 2;
        float y2 = inPos.y + inMod->getY() + inPos.h / 2;
        
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
        batcher.drawCurve(curvePoints, 3.0f * m_zoom, Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b, 1.0f); 
    }

    std::shared_ptr<FluxProject> m_project;
    AudioEngine* m_engine;
    AudioDeviceManager* m_deviceManager;
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
    std::shared_ptr<Component> m_popup;
};

} // namespace Beam

#endif