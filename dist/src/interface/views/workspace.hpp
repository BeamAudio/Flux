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
#include "engine/nodes/midi_track_node.hpp"
#include "engine/nodes/input_node.hpp"
#include "engine/nodes/midi_input_node.hpp"
#include "engine/scripting/flux_script_node.hpp"
#include "engine/nodes/analog_suite.hpp"
#include "engine/nodes/flux_fx_nodes.hpp"
#include "engine/nodes/pitch_fx_nodes.hpp"
#include "engine/core/audio_engine.hpp"
#include "engine/plugins/plugin_registry.hpp"
#include "engine/session/flux_project.hpp"
#include "engine/session/commands.hpp"
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
        // Hardware & Recording
        reg.registerPlugin("Audio Input", "Hardware", [](int b, float s){ return std::make_shared<InputNode>(b); });
        reg.registerPlugin("MIDI Input", "Hardware", [](int b, float s){ return std::make_shared<MIDIInputNode>(); });
        reg.registerPlugin("Empty Tape", "Recording", [](int b, float s){ return std::make_shared<FluxTrackNode>("Empty Tape", b); });
        reg.registerPlugin("MIDI Floppy", "Recording", [](int b, float s){ return std::make_shared<MIDITrackNode>(); });
        reg.registerPlugin("Script", "Scripting", [](int b, float s){ return nullptr; }); 

        // Dynamics
        reg.registerPlugin("Opto-2A", "Dynamics", [](int b, float s){ return std::make_shared<Opto2A>(b, s); });
        reg.registerPlugin("FET-76", "Dynamics", [](int b, float s){ return std::make_shared<FET76>(b, s); });
        reg.registerPlugin("Tube Limiter", "Dynamics", [](int b, float s){ return std::make_shared<TubeLimiter>(b, s); });
        reg.registerPlugin("Lookahead Limiter", "Dynamics", [](int b, float s){ return std::make_shared<LookaheadLimiter>(b, s); });
        reg.registerPlugin("VCA-Bus", "Dynamics", [](int b, float s){ return std::make_shared<VCABus>(b, s); });
        reg.registerPlugin("Vari-Mu", "Dynamics", [](int b, float s){ return std::make_shared<VariMu>(b, s); });
        
        // EQ
        reg.registerPlugin("Tube-P EQ", "EQ", [](int b, float s){ return std::make_shared<TubeP_EQ>(b, s); });
        reg.registerPlugin("Console-E", "EQ", [](int b, float s){ return std::make_shared<ConsoleE_EQ>(b, s); });
        reg.registerPlugin("Vintage-G", "EQ", [](int b, float s){ return std::make_shared<VintageG_EQ>(b, s); });
        reg.registerPlugin("Graphic-10", "EQ", [](int b, float s){ return std::make_shared<Graphic10_EQ>(b, s); });
        reg.registerPlugin("Air-Lift", "EQ", [](int b, float s){ return std::make_shared<AirLift_EQ>(b, s); });

        // Reverb
        reg.registerPlugin("Steel Plate", "Reverb", [](int b, float s){ return std::make_shared<SteelPlate>(b, s); });
        reg.registerPlugin("Golden Hall", "Reverb", [](int b, float s){ return std::make_shared<GoldenHall>(b, s); });
        reg.registerPlugin("Copper Spring", "Reverb", [](int b, float s){ return std::make_shared<CopperSpring>(b, s); });
        reg.registerPlugin("Cathedral", "Reverb", [](int b, float s){ return std::make_shared<Cathedral>(b, s); });
        reg.registerPlugin("Grain Verb", "Reverb", [](int b, float s){ return std::make_shared<GrainVerb>(b, s); });

        // Delay
        reg.registerPlugin("Echo-Plex", "Delay", [](int b, float s){ return std::make_shared<EchoPlex>(b, s); });
        reg.registerPlugin("BBD-Bucket", "Delay", [](int b, float s){ return std::make_shared<BBD_Bucket>(b, s); });
        reg.registerPlugin("Reverse", "Delay", [](int b, float s){ return std::make_shared<Reverse_Delay>(b, s); });
        reg.registerPlugin("Ping-Pong", "Delay", [](int b, float s){ return std::make_shared<PingPong_Delay>(b, s); });
        reg.registerPlugin("Space Shift", "Delay", [](int b, float s){ return std::make_shared<SpaceShift>(b, s); });

        // Pitch
        reg.registerPlugin("Auto-Tune", "Pitch", [](int b, float s){ return std::make_shared<AutoTuneNode>(b, s); });

        // Utilities
        reg.registerPlugin("Spectrum", "Utilities", [](int b, float s){ return std::make_shared<FluxSpectrumAnalyzer>(b, s); });
        reg.registerPlugin("Loudness", "Utilities", [](int b, float s){ return std::make_shared<FluxLoudnessMeter>(b, s); });
        reg.registerPlugin("Gain", "Utilities", [](int b, float s){ return std::make_shared<FluxGainNode>(b); });
        reg.registerPlugin("Filter", "Utilities", [](int b, float s){ return std::make_shared<FluxFilterNode>(b, s); });
        reg.registerPlugin("Delay", "Utilities", [](int b, float s){ return std::make_shared<FluxDelayNode>(b, s); });

        // Load Persistent Library
        auto& lib = PluginLibrary::get();
        for (const auto& entry : lib.getEntries()) {
            if (entry.type == "VST3") {
                reg.registerPlugin(entry.name, entry.category, [path = entry.path](int b, float s) {
                    auto node = std::make_shared<VST3HostNode>(path);
                    if (node->load()) return node;
                    return std::shared_ptr<VST3HostNode>(nullptr);
                });
            } else if (entry.type == "FluxScript") {
                reg.registerPlugin(entry.name, entry.category, [path = entry.path](int b, float s) {
                    return FluxCompiler::loadPlugin(path, b, s);
                });
            }
        }
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

    void childBoundsChanged(Component* child) override {
        if (m_project && m_isDragging) {
             m_project->setDirty(true);
        }
        Component::childBoundsChanged(child);
    }

    void syncReels() {
        if (!m_project) return;
        auto& tracks = m_project->getTracks();
        auto nodes = m_project->getGraph()->getNodes();

        // 1. Remove Ghost Modules (Modules not in Graph)
        for (auto it = m_modules.begin(); it != m_modules.end(); ) {
            size_t id = (*it)->getNodeId();
            if (nodes.find(id) == nodes.end()) {
                // Remove visual component
                removeChildComponent(it->get());
                it = m_modules.erase(it);
            } else {
                ++it;
            }
        }
        
        // 2. Add Tracks/Reels
        for (auto& track : tracks) {
            bool exists = false;
            for (auto& mod : m_modules) {
                auto reel = std::dynamic_pointer_cast<TapeReel>(mod);
                if (reel && reel->getNodeId() == track.nodeId) {
                    if (reel->getNode() != track.node) reel->setNode(track.node);
                    exists = true; 
                    break; 
                }
            }
            if (!exists) {
                // Restore position from visuals, fallback to defaults
                auto [vx, vy] = m_project->getVisualPos(track.nodeId);
                if (vx == 100.0f && vy == 100.0f) {
                    // Default position if not saved
                    vx = 400.0f + (track.trackIndex * 50.0f);
                    vy = 100.0f + (track.trackIndex * 150.0f);
                }
                auto reel = std::make_shared<TapeReel>(track.node, track.nodeId, vx, vy);
                setupModule(reel);
            }
        }

        // 3. Sync Standard Nodes (Exclude Track Nodes - handled above)
        // nodes variable already populated above
        for (auto const& [id, node] : nodes) {
            // Skip FluxTrackNodes - they should be TapeReels from step 2
            if (std::dynamic_pointer_cast<FluxTrackNode>(node)) continue;
            
            bool exists = false;
            for (auto& mod : m_modules) {
                if (mod->getNodeId() == id) { 
                    exists = true; 
                    if (mod->getNode() != node) mod->setNode(node);
                    break; 
                }
            }
            if (!exists) {
                // Restore position if available
                auto [vx, vy] = m_project->getVisualPos(id);
                // If default (100,100) and it's Master, verify master defaults
                if (vx == 100.0f && vy == 100.0f) {
                     if (node->getName() == "Master") { vx = 800.0f; vy = 250.0f; }
                }
                
                auto mod = std::make_shared<AudioModule>(node, id, vx, vy, m_deviceManager);
                mod->setDraggable(true);
                setupModule(mod);
            }
        }

        
        syncCables();
    }

    void syncCables() {
        if (!m_project) return;
        m_cables.clear();
        
        auto conns = m_project->getGraph()->getConnections();
        for (const auto& c : conns) {
            AudioModule* srcMod = nullptr;
            for (auto& m : m_modules) if (m->getNodeId() == c.srcNodeId) { srcMod = m.get(); break; }
            
            AudioModule* dstMod = nullptr;
            for (auto& m : m_modules) if (m->getNodeId() == c.dstNodeId) { dstMod = m.get(); break; }
            
            if (srcMod && dstMod) {
                // Find Out Port by Index
                Port* outPort = nullptr;
                for (auto& p : srcMod->getOutputPorts()) {
                    if (p->getIndex() == c.srcPortIdx) { outPort = p.get(); break; }
                }

                // Find In Port by Index
                Port* inPort = nullptr;
                for (auto& p : dstMod->getInputPorts()) {
                    if (p->getIndex() == c.dstPortIdx) { inPort = p.get(); break; }
                }
                
                if (outPort && inPort) {
                    m_cables.push_back({outPort, inPort});
                }
            }
        }
    }

    void refresh() {
        syncReels();
        // syncCables is called by syncReels now
    }

    void saveStateToProject() {
        if (!m_project) return;
        std::cout << "[Workspace] Saving state for " << m_modules.size() << " modules." << std::endl;
        // Save Module Positions
        for (auto& mod : m_modules) {
            m_project->setVisualPos(mod->getNodeId(), mod->getX(), mod->getY());
        }
    }

    void clear() {
        std::cout << "[Workspace] Clearing all modules and cables." << std::endl;
        for (auto& mod : m_modules) {
            removeChildComponent(mod.get());
        }
        m_modules.clear();
        m_cables.clear();
        m_activePort = nullptr;
        m_isDraggingCable = false;
        closePopup();
    }

    void setupModule(std::shared_ptr<AudioModule> mod) {
        mod->onDeleteRequested = [this](AudioModule* m) { removeModule(m); };
        for (auto& p : mod->getInputPorts()) p->onConnectStarted = [this](Port* p) { startCableDrag(p); };
        for (auto& p : mod->getOutputPorts()) p->onConnectStarted = [this](Port* p) { startCableDrag(p); };
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
            size_t nodeId = m_project->getGraph()->reserveNextId();
            UndoManager::get().perform(std::make_unique<AddNodeCommand>(m_project->getGraph().get(), fluxTrack, nodeId));
            
            TrackData td;
            td.node = fluxTrack;
            td.nodeId = nodeId;
            td.trackIndex = (int)m_project->getTracks().size();
            
            size_t totalFrames = fluxTrack->getInternalNode()->getTotalFrames();
            Region r = {fileName, 0, totalFrames, 0, td.trackIndex};
            r.channelPeaks = fluxTrack->getPeakData(400); 
            td.regions.push_back(r); 
            
            m_project->addTrack(td);
            m_project->setDirty(true);
            syncReels(); 
        }
        m_isLoading = false;
    }

    void addFX(const std::string& type, float x, float y) {
        int buf = 1024 * 4;
        float sr = 44100.0f;

        if (type == "Empty Tape") {
            auto fluxTrack = std::make_shared<FluxTrackNode>("Empty Tape", buf);
            size_t nodeId = m_project->getGraph()->reserveNextId();
            UndoManager::get().perform(std::make_unique<AddNodeCommand>(m_project->getGraph().get(), fluxTrack, nodeId));
            
            TrackData td; td.node = fluxTrack; td.nodeId = nodeId; td.trackIndex = (int)m_project->getTracks().size();
            m_project->addTrack(td); 
        } else if (type == "Audio Input") {
            auto node = std::make_shared<InputNode>(buf);
            size_t nodeId = m_project->getGraph()->reserveNextId();
            UndoManager::get().perform(std::make_unique<AddNodeCommand>(m_project->getGraph().get(), node, nodeId));
        } else {
            auto node = PluginRegistry::get().createPlugin(type, buf, sr);
            if (node) {
                size_t nodeId = m_project->getGraph()->reserveNextId();
                UndoManager::get().perform(std::make_unique<AddNodeCommand>(m_project->getGraph().get(), node, nodeId));
            } else {
                std::cout << "Error: Unknown FX type '" << type << "'" << std::endl;
            }
        }

        syncReels();
        m_project->setDirty(true);
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
        m_project->setDirty(true);
        if (m_engine) m_engine->updatePlan();
    }

    void removeModule(AudioModule* mod) {
        if (!mod) return;
        
        if (m_activePort && m_activePort->getParent() == mod) {
            m_activePort = nullptr;
            m_isDraggingCable = false;
        }

        size_t id = mod->getNodeId();
        UndoManager::get().perform(std::make_unique<RemoveNodeCommand>(m_project->getGraph().get(), id));
        
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
        m_project->setDirty(true);
    }

    void startCableDrag(Port* p) { 
        for (auto it = m_cables.begin(); it != m_cables.end(); ++it) {
            if (it->input == p || it->output == p) {
                auto* outMod = dynamic_cast<AudioModule*>(it->output->getParent());
                auto* inMod = dynamic_cast<AudioModule*>(it->input->getParent());
                
                if (outMod && inMod) {
                    UndoManager::get().perform(std::make_unique<DisconnectCommand>(m_project->getGraph().get(), 
                        outMod->getNodeId(), it->output->getIndex(), 
                        inMod->getNodeId(), it->input->getIndex()));
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

    void connectPorts(Port* p1, Port* p2) {
        if (!p1 || !p2 || p1->getType() == p2->getType() || !m_engine) return;
        Port* out = (p1->getType() == PortType::Output) ? p1 : p2;
        Port* in = (p1->getType() == PortType::Input || p1->getType() == PortType::Sidechain) ? p1 : p2;
        m_cables.push_back({out, in});
        
        auto* outMod = dynamic_cast<AudioModule*>(out->getParent());
        auto* inMod = dynamic_cast<AudioModule*>(in->getParent());

        if(outMod && inMod) {
            UndoManager::get().perform(std::make_unique<ConnectCommand>(m_project->getGraph().get(),
                outMod->getNodeId(), out->getIndex(), inMod->getNodeId(), in->getIndex()));
        }
        m_project->setDirty(true);
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
                auto checkPorts = [&](std::vector<std::shared_ptr<Port>>& ports) -> bool {
                    for (auto& p : ports) {
                        Rect b = p->getBounds(); 
                        float padding = 15.0f / m_zoom;
                        float px = mod->getX() + b.x;
                        float py = mod->getY() + b.y;
                        if (vmx >= px - padding && vmx <= px + b.w + padding && vmy >= py - padding && vmy <= py + b.h + padding) {
                            connectPorts(m_activePort, p.get());
                            return true;
                        }
                    }
                    return false;
                };
                if (checkPorts(mod->getInputPorts())) break;
                if (checkPorts(mod->getOutputPorts())) break;
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