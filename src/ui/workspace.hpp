#ifndef WORKSPACE_HPP
#define WORKSPACE_HPP

#include "component.hpp"
#include "tape_reel.hpp"
#include "cable.hpp"
#include "../dsp/flux_track_node.hpp"
#include "../dsp/flux_fx_nodes.hpp"
#include "../dsp/custom_filter.hpp"
#include "../dsp/audio_engine.hpp"
#include "../core/flux_project.hpp"
#include <vector>
#include <iostream>

#include "json.hpp"

namespace Beam {

class Workspace : public Component {
public:
    Workspace(std::shared_ptr<FluxProject> project) : m_project(project) {
        setBounds(0, 0, 10000, 10000); 
    }

    nlohmann::json serialize() const {
        nlohmann::json data;
        data["panX"] = m_panX;
        data["panY"] = m_panY;
        
        nlohmann::json modules = nlohmann::json::array();
        // Here we would iterate modules and call their serialize()
        data["modules"] = modules;
        
        return data;
    }

    void deserialize(const nlohmann::json& data) {
        if (data.contains("panX")) m_panX = data["panX"];
        if (data.contains("panY")) m_panY = data["panY"];
        // In a real implementation, we'd reconstruct modules here
    }

    void setVisible(bool visible) { m_isVisible = visible; }

    void render(QuadBatcher& batcher) override {
        if (!m_isVisible) return;
        // Draw Grid
        float spacing = 50.0f;
        for (float x = 0; x < 2000; x += spacing) {
            batcher.drawQuad(x + m_panX, 0, 1, 2000, 0.2f, 0.2f, 0.2f, 1.0f);
        }
        for (float y = 0; y < 2000; y += spacing) {
            batcher.drawQuad(0, y + m_panY, 2000, 1, 0.2f, 0.2f, 0.2f, 1.0f);
        }

        // Draw Cables
        for (auto& cable : m_cables) {
            cable.render(batcher);
        }

        // Draw Dragging Cable
        if (m_isDraggingCable && m_activePort) {
            Rect pos = m_activePort->getBounds();
            batcher.drawQuad(pos.x + 6, pos.y + 6, 4, 4, 1.0f, 1.0f, 1.0f, 0.5f);
        }

        for (auto& module : m_modules) {
            module->render(batcher);
        }
    }

    void addTrack(const std::string& filePath, float x, float y, AudioEngine& engine) {
        auto graph = m_project->getGraph();
        auto fluxTrack = std::make_shared<FluxTrackNode>("Loaded Track", 1024 * 4);
        
        if (fluxTrack->load(filePath)) {
            fluxTrack->getInternalNode()->setState(TrackState::Playing);
            size_t nodeId = graph->addNode(fluxTrack);
            
            // Auto-connect to Master for now (Master is node 0 in this simplified update)
            graph->connect(nodeId, 0, 0, 0); 

            auto reel = std::make_shared<TapeReel>(fluxTrack, x, y);
            
            reel->getInputPort()->onConnectStarted = [this](Port* p) { startCableDrag(p); };
            reel->getOutputPort()->onConnectStarted = [this](Port* p) { startCableDrag(p); };

            m_modules.push_back(reel);
        }
    }

    void addFX(const std::string& type, float x, float y) {
        auto graph = m_project->getGraph();
        std::shared_ptr<FluxNode> fxNode;
        
        if (type == "Gain") fxNode = std::make_shared<FluxGainNode>(1024 * 4);
        else if (type == "Filter") fxNode = std::make_shared<FluxFilterNode>(1024 * 4, 44100.0f);
        else if (type == "Delay") fxNode = std::make_shared<FluxDelayNode>(1024 * 4, 44100.0f);
        else if (type == "Custom") fxNode = std::make_shared<CustomFilter>(1024 * 4, 44100.0f);
        
        if (fxNode) {
            graph->addNode(fxNode);
            auto mod = std::make_shared<AudioModule>(fxNode, x, y);
            m_modules.push_back(mod);
        }
    }

    void startCableDrag(Port* p) {
        m_isDraggingCable = true;
        m_activePort = p;
    }

    void connectPorts(Port* p1, Port* p2) {
        if (!p1 || !p2 || p1->getType() == p2->getType()) return;
        Port* out = (p1->getType() == PortType::Output) ? p1 : p2;
        Port* in = (p1->getType() == PortType::Input) ? p1 : p2;
        m_cables.push_back({out, in});
    }

    bool onMouseDown(float x, float y, int button) override {
        for (auto it = m_modules.rbegin(); it != m_modules.rend(); ++it) {
            if ((*it)->getBounds().contains(x, y)) {
                return (*it)->onMouseDown(x, y, button);
            }
        }
        
        if (button == 2) { 
            m_isPanning = true;
            m_lastMouseX = x;
            m_lastMouseY = y;
            return true;
        }
        return false;
    }

    bool onMouseUp(float x, float y, int button) override {
        if (m_isDraggingCable) {
            for (auto& mod : m_modules) {
                auto audioMod = std::dynamic_pointer_cast<AudioModule>(mod);
                if (audioMod) {
                    if (audioMod->getInputPort()->getBounds().contains(x, y)) {
                        connectPorts(m_activePort, audioMod->getInputPort().get());
                    } else if (audioMod->getOutputPort()->getBounds().contains(x, y)) {
                        connectPorts(m_activePort, audioMod->getOutputPort().get());
                    }
                }
            }
            m_isDraggingCable = false;
            m_activePort = nullptr;
        }
        m_isPanning = false;
        for (auto& mod : m_modules) mod->onMouseUp(x, y, button);
        return true;
    }

    bool onMouseMove(float x, float y) override {
        if (m_isPanning) {
            m_panX += (x - m_lastMouseX);
            m_panY += (y - m_lastMouseY);
            m_lastMouseX = x;
            m_lastMouseY = y;
            return true;
        }
        for (auto& mod : m_modules) {
            if (mod->onMouseMove(x, y)) return true;
        }
        return false;
    }

private:
    std::shared_ptr<FluxProject> m_project;
    std::vector<std::shared_ptr<Component>> m_modules;
    std::vector<Cable> m_cables;
    float m_panX = 0, m_panY = 0;
    bool m_isPanning = false;
    float m_lastMouseX = 0, m_lastMouseY = 0;
    bool m_isDraggingCable = false;
    Port* m_activePort = nullptr;
};

} // namespace Beam

#endif // WORKSPACE_HPP
