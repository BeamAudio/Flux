#ifndef WORKSPACE_HPP
#define WORKSPACE_HPP

#include "component.hpp"
#include "tape_reel.hpp"
#include "cable.hpp"
#include "../engine/flux_track_node.hpp"
#include "../engine/flux_fx_nodes.hpp"
#include "../engine/audio_engine.hpp"
#include "../session/flux_project.hpp"
#include <vector>
#include <iostream>

namespace Beam {

class Workspace : public Component {
public:
    Workspace(std::shared_ptr<FluxProject> project, AudioEngine* engine) 
        : m_project(project), m_engine(engine) {
        setBounds(0, 0, 10000, 10000); 
    }

    void update(float dt) override {
        syncReels();
        for (auto& module : m_modules) module->update(dt);
    }

    void render(QuadBatcher& batcher, float dt) override {
        if (!m_isVisible) return;
        
        float spacing = 50.0f;
        for (float x = 0; x < 2000; x += spacing) batcher.drawQuad(x + m_panX, 0, 1, 2000, 0.2f, 0.2f, 0.2f, 1.0f);
        for (float y = 0; y < 2000; y += spacing) batcher.drawQuad(0, y + m_panY, 2000, 1, 0.2f, 0.2f, 0.2f, 1.0f);

        for (auto& cable : m_cables) cable.render(batcher, dt);

        if (m_isDraggingCable && m_activePort) {
            float mx, my; SDL_GetMouseState(&mx, &my);
            Rect pB = m_activePort->getBounds();
            batcher.drawLine(pB.x + 6, pB.y + 6, mx, my, 2.0f, 1.0f, 0.8f, 0.2f, 0.8f);
        }

        for (auto& module : m_modules) module->render(batcher, dt);

        if (m_isLoading) {
            m_loadingTimer += dt;
            float cx = m_bounds.x + m_bounds.w * 0.5f;
            float cy = m_bounds.y + m_bounds.h * 0.5f;
            
            // Draw a spinning reel icon as loading indicator
            for (int i = 0; i < 4; ++i) {
                float angle = m_loadingTimer * 5.0f + (i * 1.57f);
                float rx = cx + std::cos(angle) * 20.0f;
                float ry = cy + std::sin(angle) * 20.0f;
                batcher.drawRoundedRect(rx - 5, ry - 5, 10, 10, 5.0f, 0.5f, 0.2f, 0.5f, 1.0f, 1.0f);
            }
            batcher.drawText("LOADING TAPE...", cx - 40, cy + 40, 12, 1.0f, 1.0f, 1.0f, 1.0f);
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
                auto reel = std::make_shared<TapeReel>(track.node, track.nodeId, 400.0f + (track.trackIndex * 50.0f), 100.0f + (track.trackIndex * 150.0f));
                setupModule(reel);
            }
        }

        // Sync Input Node
        auto nodes = m_project->getGraph()->getNodes();
        for (auto const& [id, node] : nodes) {
            if (node->getName() == "Audio Input") {
                bool exists = false;
                for (auto& mod : m_modules) {
                    auto audioMod = std::dynamic_pointer_cast<AudioModule>(mod);
                    if (audioMod && audioMod->getNodeId() == id) { exists = true; break; }
                }
                if (!exists) {
                    auto mod = std::make_shared<AudioModule>(node, id, 100.0f, 100.0f);
                    setupModule(mod);
                }
            }
            else if (node->getName() == "Master") {
                bool exists = false;
                for (auto& mod : m_modules) {
                    auto audioMod = std::dynamic_pointer_cast<AudioModule>(mod);
                    if (audioMod && audioMod->getNodeId() == id) { exists = true; break; }
                }
                if (!exists) {
                    auto mod = std::make_shared<AudioModule>(node, id, 800.0f, 250.0f);
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
        std::cout << "Loading file: " << filePath << std::endl;
        
        // Extract file name from path
        std::string fileName = filePath;
        size_t lastSlash = filePath.find_last_of("/\\");
        if (lastSlash != std::string::npos) fileName = filePath.substr(lastSlash + 1);

        auto fluxTrack = std::make_shared<FluxTrackNode>(fileName, 1024 * 4);
        if (fluxTrack->load(filePath)) {
            size_t nodeId = m_project->getGraph()->addNode(fluxTrack);
            // engine.updatePlan(); // Redundant if addTrack doesn't auto-connect now
            
            TrackData td;
            td.node = fluxTrack;
            td.nodeId = nodeId;
            td.trackIndex = (int)m_project->getTracks().size();
            
            size_t totalFrames = fluxTrack->getInternalNode()->getTotalFrames();
            Region r = {fileName, 0, totalFrames, 0, td.trackIndex};
            r.channelPeaks = fluxTrack->getPeakData(400); // More points for better resolution
            td.regions.push_back(r);
            
            m_project->addTrack(td);
            syncReels(); // Immediate UI update
            std::cout << "Track added: " << fileName << " (" << totalFrames << " frames)" << std::endl;
        }
        m_isLoading = false;
    }

    void addFX(const std::string& type, float x, float y) {
        std::shared_ptr<FluxNode> fxNode;
        if (type == "Gain") fxNode = std::make_shared<FluxGainNode>(1024 * 4);
        else if (type == "Filter") fxNode = std::make_shared<FluxFilterNode>(1024 * 4, 44100.0f);
        else if (type == "Delay") fxNode = std::make_shared<FluxDelayNode>(1024 * 4, 44100.0f);
        if (fxNode) {
            size_t id = m_project->getGraph()->addNode(fxNode);
            auto mod = std::make_shared<AudioModule>(fxNode, id, x, y);
            setupModule(mod);
            syncReels(); // Immediate UI update
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

    void startCableDrag(Port* p) { m_isDraggingCable = true; m_activePort = p; }

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
        for (auto it = m_modules.rbegin(); it != m_modules.rend(); ++it) {
            if ((*it)->getBounds().contains(x, y)) return (*it)->onMouseDown(x, y, button);
        }
        if (button == 2) { m_isPanning = true; m_lastMouseX = x; m_lastMouseY = y; return true; }
        return false;
    }

    bool onMouseUp(float x, float y, int button) override {
        if (m_isDraggingCable) {
            for (auto& mod : m_modules) {
                auto audioMod = std::dynamic_pointer_cast<AudioModule>(mod);
                if (audioMod) {
                    // Fuzzy hit detection for ports (with safety checks)
                    auto checkPort = [&](std::shared_ptr<Port> p) {
                        if (!p) return false;
                        Rect b = p->getBounds();
                        float padding = 15.0f; 
                        return (x >= b.x - padding && x <= b.x + b.w + padding && y >= b.y - padding && y <= b.y + b.h + padding);
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
        if (m_isDraggingCable) return true; // Handled in render
        if (!m_isVisible) return false;
        if (m_isPanning) {
            m_panX += (x - m_lastMouseX); m_panY += (y - m_lastMouseY);
            m_lastMouseX = x; m_lastMouseY = y; return true;
        }
        for (auto& mod : m_modules) if (mod->onMouseMove(x, y)) return true;
        return false;
    }

    void setVisible(bool visible) { m_isVisible = visible; }

private:
    std::shared_ptr<FluxProject> m_project;
    AudioEngine* m_engine;
    std::vector<std::shared_ptr<Component>> m_modules;
    std::vector<Cable> m_cables;
    float m_panX = 0, m_panY = 0;
    bool m_isPanning = false;
    float m_lastMouseX = 0, m_lastMouseY = 0;
    bool m_isDraggingCable = false;
    Port* m_activePort = nullptr;
    bool m_isLoading = false;
    float m_loadingTimer = 0.0f;
};

} // namespace Beam

#endif // WORKSPACE_HPP


