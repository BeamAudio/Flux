#ifndef WORKSPACE_HPP
#define WORKSPACE_HPP

#include "component.hpp"
#include "tape_reel.hpp"
#include "cable.hpp"
#include "filter_module.hpp"
#include "dynamics_module.hpp"
#include "spectrum_module.hpp"
#include "loudness_module.hpp"
#include "popup_host.hpp"
#include "../engine/flux_track_node.hpp"
#include "../engine/flux_script_node.hpp"
#include "../engine/analog_suite.hpp"
#include "../engine/flux_fx_nodes.hpp"
#include "../engine/audio_engine.hpp"
#include "../engine/audio_device_manager.hpp"
#include "../engine/plugin_registry.hpp"
#include "../session/flux_project.hpp"
#include "../utilities/flux_audio_utils.hpp"
#include <vector>
#include <iostream>
#include <cmath>
#include <SDL3/SDL.h> // Explicitly include SDL for mouse state

namespace Beam {

class Workspace : public Component, public PopupHost {
public:
    Workspace(std::shared_ptr<FluxProject> project, AudioEngine* engine, AudioDeviceManager* deviceManager = nullptr) 
        : m_project(project), m_engine(engine), m_deviceManager(deviceManager) {
        setName("Workspace");
        setBounds(0, 0, 10000, 10000); 
        m_zoom = 1.0f;

        registerBuiltInPlugins();
    }

    void registerBuiltInPlugins() {
        auto& reg = PluginRegistry::get();
        // Dynamics
        reg.registerPlugin("Opto-2A", [](int b, float s){ return std::make_shared<Opto2A>(b, s); });
        reg.registerPlugin("FET-76", [](int b, float s){ return std::make_shared<FET76>(b, s); });
        reg.registerPlugin("Tube Limiter", [](int b, float s){ return std::make_shared<TubeLimiter>(b, s); });
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

        // Utility
        reg.registerPlugin("Gain", [](int b, float s){ return std::make_shared<FluxGainNode>(b); });
        reg.registerPlugin("Filter", [](int b, float s){ return std::make_shared<FluxFilterNode>(b, s); });
        reg.registerPlugin("Delay", [](int b, float s){ return std::make_shared<FluxDelayNode>(b, s); });
        reg.registerPlugin("Spectrum", [](int b, float s){ return std::make_shared<FluxSpectrumAnalyzer>(b, s); });
        reg.registerPlugin("Loudness", [](int b, float s){ return std::make_shared<FluxLoudnessMeter>(b, s); });
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

    void paint(QuadBatcher& batcher) override {
        // Background Grid (Virtual Space) - Special logic needing zoom
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        if (!m_isVisible) return; 
        
        batcher.setViewTransform(m_panX, m_panY, m_zoom);

        // Background Grid
        float spacing = 50.0f;
        for (float x = 0; x < 5000; x += spacing) 
            batcher.drawQuad(x, 0, 1, 5000, 0.2f, 0.2f, 0.2f, 0.5f);
        for (float y = 0; y < 5000; y += spacing) 
            batcher.drawQuad(0, y, 5000, 1, 0.2f, 0.2f, 0.2f, 0.5f);

        for (auto& cable : m_cables) renderCable(batcher, cable, dt, screenH);

        Component::render(batcher, dt, screenW, screenH);

        if (m_popup) {
            // Popups are rendered in screen space (or Workspace root space) on top of everything
            // Note: Component::render pops the clip, so we are unclipped here relative to Workspace
            m_popup->render(batcher, dt, screenW, screenH);
        }

        batcher.resetViewTransform(screenW, screenH);

        if (m_isDraggingCable && m_activePort) {
            float mx, my;
            SDL_GetMouseState(&mx, &my);
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
                batcher.drawRoundedRect(rx - 8, ry - 8, 16, 16, 8.0f, 0.5f, 0.13f, 0.62f, 0.42f, 1.0f); // BRAND_EMERALD
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
            if (node->getName() == "Audio Input" || node->getName() == "Master") {
                bool exists = false;
                for (auto& mod : m_modules) {
                    auto audioMod = std::dynamic_pointer_cast<AudioModule>(mod);
                    if (audioMod && audioMod->getNodeId() == id) { exists = true; break; }
                }
                if (!exists) {
                    float x = (node->getName() == "Master" ? 800.0f : 100.0f);
                    float y = (node->getName() == "Master" ? 250.0f : 100.0f);
                    auto mod = std::make_shared<AudioModule>(node, id, x, y, m_deviceManager);
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
        float vx = (x - m_panX) / m_zoom;
        float vy = (y - m_panY) / m_zoom;

        if (type == "Empty Tape") {
            auto fluxTrack = std::make_shared<FluxTrackNode>("Empty Tape", buf);
            size_t nodeId = m_project->getGraph()->addNode(fluxTrack);
            TrackData td; td.node = fluxTrack; td.nodeId = nodeId; td.trackIndex = (int)m_project->getTracks().size();
            m_project->addTrack(td); syncReels(); return;
        }

        // Use Registry
        auto node = PluginRegistry::get().createPlugin(type, buf, sr);
        
        if (node) {
            size_t id = m_project->getGraph()->addNode(node);
            std::shared_ptr<AudioModule> mod;

            // Specialized UI mappings
            if (type == "Filter") {
                if (auto cast = std::dynamic_pointer_cast<FluxFilterNode>(node)) 
                    mod = std::make_shared<FilterModule>(cast, id, vx, vy);
            }
            else if (type == "Opto-2A") {
                if (auto cast = std::dynamic_pointer_cast<Opto2A>(node)) 
                    mod = std::make_shared<DynamicsModule<Opto2A>>(cast, id, vx, vy);
            }
            else if (type == "FET-76") {
                if (auto cast = std::dynamic_pointer_cast<FET76>(node)) 
                    mod = std::make_shared<DynamicsModule<FET76>>(cast, id, vx, vy);
            }
            else if (type == "Tube Limiter") {
                if (auto cast = std::dynamic_pointer_cast<TubeLimiter>(node)) 
                    mod = std::make_shared<DynamicsModule<TubeLimiter>>(cast, id, vx, vy);
            }
            else if (type == "VCA-Bus") {
                if (auto cast = std::dynamic_pointer_cast<VCABus>(node)) 
                    mod = std::make_shared<DynamicsModule<VCABus>>(cast, id, vx, vy);
            }
            else if (type == "Vari-Mu") {
                if (auto cast = std::dynamic_pointer_cast<VariMu>(node)) 
                    mod = std::make_shared<DynamicsModule<VariMu>>(cast, id, vx, vy);
            }
            else if (type == "Spectrum") {
                if (auto cast = std::dynamic_pointer_cast<FluxSpectrumAnalyzer>(node))
                    mod = std::make_shared<SpectrumModule>(cast, id, vx, vy);
            }
            else if (type == "Loudness") {
                if (auto cast = std::dynamic_pointer_cast<FluxLoudnessMeter>(node))
                    mod = std::make_shared<LoudnessModule>(cast, id, vx, vy);
            }
            
            // Fallback to auto-generated
            if (!mod) mod = std::make_shared<AudioModule>(node, id, vx, vy, m_deviceManager); 

            setupModule(mod);
            syncReels();
            if (m_engine) m_engine->updatePlan();
        } else {
            std::cout << "Error: Unknown FX type '" << type << "'" << std::endl;
        }
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
            if (it->get() == mod) { 
                removeChildComponent(it->get());
                m_modules.erase(it); 
                break; 
            }
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
        m_isDraggingCable = true; m_activePort = p; 
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
        
        if (m_popup) {
            float vmx = (x - m_panX) / m_zoom;
            float vmy = (y - m_panY) / m_zoom;
            if (m_popup->getBounds().contains(vmx, vmy)) {
                return m_popup->onMouseDown(vmx, vmy, button);
            } else {
                closePopup();
                return true; 
            }
        }

        float vmx = (x - m_panX) / m_zoom;
        float vmy = (y - m_panY) / m_zoom;
        
        if (Component::onMouseDown(vmx, vmy, button)) return true;

        if (button == 3) { m_isPanning = true; m_lastMouseX = x; m_lastMouseY = y; return true; }
        return false;
    }

    bool onMouseUp(float x, float y, int button) override {
        if (m_popup) {
             return true; 
        }

        float vmx = (x - m_panX) / m_zoom;
        float vmy = (y - m_panY) / m_zoom;

        if (m_isDraggingCable) {
            for (auto& mod : m_modules) {
                auto checkPort = [&](std::shared_ptr<Port> p) {
                    if (!p) return false;
                    Rect b = p->getBounds();
                    float padding = 15.0f / m_zoom;
                    return (vmx >= b.x - padding && vmx <= b.x + b.w + padding && vmy >= b.y - padding && vmy <= b.y + b.h + padding);
                };
                if (checkPort(mod->getInputPort())) connectPorts(m_activePort, mod->getInputPort().get());
                else if (checkPort(mod->getOutputPort())) connectPorts(m_activePort, mod->getOutputPort().get());
            }
            m_isDraggingCable = false; m_activePort = nullptr; return true;
        }
        m_isPanning = false;
        
        if (Component::onMouseUp(vmx, vmy, button)) return true;
        
        return true;
    }

    bool onMouseMove(float x, float y) override {
        float vmx = (x - m_panX) / m_zoom;
        float vmy = (y - m_panY) / m_zoom;

        if (m_popup) {
            m_popup->onMouseMove(vmx, vmy);
            return true;
        }

        if (m_isDraggingCable) return true; 
        if (!m_isVisible) return false;
        if (m_isPanning) {
            m_panX += (x - m_lastMouseX); 
            m_panY += (y - m_lastMouseY);
            m_lastMouseX = x; m_lastMouseY = y; return true;
        }
        
        if (Component::onMouseMove(vmx, vmy)) return true;
        
        return false;
    }

    bool onMouseWheel(float x, float y, float delta) override {
        if (!m_isVisible) return false;
        float oldZoom = m_zoom;
        m_zoom *= (delta > 0) ? 1.1f : 0.9f;
        m_zoom = (std::clamp)(m_zoom, 0.1f, 5.0f);
        m_panX = x - (x - m_panX) * (m_zoom / oldZoom);
        m_panY = y - (y - m_panY) * (m_zoom / oldZoom);
        return true;
    }

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
        batcher.drawCurve(curvePoints, 3.0f * m_zoom, 0.13f, 0.62f, 0.42f, 1.0f); // Emerald
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
