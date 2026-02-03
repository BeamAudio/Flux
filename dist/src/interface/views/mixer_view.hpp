#ifndef MIXER_VIEW_HPP
#define MIXER_VIEW_HPP

#include "interface/core/component.hpp"
#include "interface/views/channel_strip.hpp"
#include "interface/views/master_strip.hpp"
#include "engine/session/flux_project.hpp"
#include "engine/core/audio_engine.hpp"
#include <vector>

namespace Beam {

/**
 * @class MixerView
 * @brief A classic vertical mixer displaying channel strips for nodes connected to Master.
 * 
 * The mixer controls internal gain stages within the Master node's summing bus via MixerState.
 */
class MixerView : public Component {
public:
    MixerView(std::shared_ptr<FluxProject> project, AudioEngine* engine)
        : m_project(project), m_engine(engine)
    {
        setName("MixerView");
        
        // Master Strip on the right
        m_masterStrip = std::make_shared<MasterStrip>(engine);
        addChildComponent(m_masterStrip);
    }
    
    /**
     * @brief Rebuilds channel strips based on current graph connections.
     * Called when switching to Mix mode or when connections change.
     */
    std::string resolveTrackName(size_t nodeId) {
        if (!m_project) return "";
        const auto& tracks = m_project->getTracks();
        
        // 1. Check direct match
        for (const auto& t : tracks) {
            if (t.nodeId == nodeId) return "Track " + std::to_string(t.trackIndex);
        }
        
        // 2. Walk upstream BFS
        if (!m_project->getGraph()) return "";
        auto g = m_project->getGraph();
        auto connections = g->getConnections();
        
        std::vector<size_t> queue = {nodeId};
        std::vector<size_t> visited = {nodeId};
        size_t head = 0;
        
        while(head < queue.size() && head < 50) { 
            size_t curr = queue[head++];
            for (const auto& t : tracks) {
                if (t.nodeId == curr) return "Track " + std::to_string(t.trackIndex);
            }
            
            // Add inputs
            for (const auto& c : connections) {
                if (c.dstNodeId == curr) {
                     bool seen = false;
                     for(size_t v : visited) if(v==c.srcNodeId) seen=true;
                     if(!seen) {
                         visited.push_back(c.srcNodeId);
                         queue.push_back(c.srcNodeId);
                     }
                }
            }
        }
        return "";
    }

    void refresh() {
        // Clear existing channel strips
        for (auto& strip : m_channelStrips) {
            removeChildComponent(strip.get());
        }
        m_channelStrips.clear();
        
        if (!m_project || !m_project->getGraph() || !m_engine) return;
        
        auto graph = m_project->getGraph();
        size_t masterNodeId = m_engine->getMasterNodeId();
        MixerState& mixerState = m_engine->getMixerState();
        
        // Find all connections going into Master
        auto connections = graph->getConnections();
        std::vector<size_t> sourceNodeIds;
        
        for (const auto& conn : connections) {
            if (conn.dstNodeId == masterNodeId) {
                // Avoid duplicates
                bool found = false;
                for (auto id : sourceNodeIds) {
                    if (id == conn.srcNodeId) { found = true; break; }
                }
                if (!found) {
                    sourceNodeIds.push_back(conn.srcNodeId);
                }
            }
        }
        
        // Create a channel strip for each source node
        auto& nodes = graph->getNodes();
        for (size_t srcId : sourceNodeIds) {
            auto it = nodes.find(srcId);
            if (it == nodes.end()) continue;
            
            auto node = it->second;
            std::string name = resolveTrackName(srcId);
            if (name.empty()) name = node->getName();
            
            // Get or create MixChannel for this node
            MixChannel* channel = mixerState.getOrCreateChannel(srcId);
            
            // Create channel strip
            auto strip = std::make_shared<ChannelStrip>(name, channel, srcId, node);
            m_channelStrips.push_back(strip);
            addChildComponent(strip);
        }
        
        performLayout();
    }
    
    void setBounds(float x, float y, float width, float height) override {
        Component::setBounds(x, y, width, height);
        performLayout();
    }
    
    void performLayout() {
        float padding = 8.0f;
        float stripWidth = 80.0f;
        float masterWidth = 140.0f;
        float stripHeight = m_bounds.h - 2*padding;
        
        // Channel strips left to right
        float currentX = padding;
        for (auto& strip : m_channelStrips) {
            strip->setBounds(currentX, padding, stripWidth, stripHeight);
            currentX += stripWidth + padding;
        }
        
        // Master strip on the right
        m_masterStrip->setBounds(m_bounds.w - masterWidth - padding, padding, masterWidth, stripHeight);
    }
    
    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        // Update meters from MixChannel peak levels (written by audio thread)
        if (m_engine) {
            MixerState& mixerState = m_engine->getMixerState();
            for (auto& strip : m_channelStrips) {
                size_t nodeId = strip->getNodeId();
                MixChannel* ch = mixerState.getChannel(nodeId);
                if (ch) {
                    float peak = ch->peakL.load(std::memory_order_relaxed);
                    strip->setMeterLevel(peak);
                }
            }
        }
        
        Component::render(batcher, dt, screenW, screenH);
    }
    
    void paint(QuadBatcher& batcher) override {
        // Dark console background
        batcher.drawChassisPanel(0, 0, m_bounds.w, m_bounds.h, 0.0f, 0.06f, 0.06f, 0.07f, 1.0f);
        
        // Title plate at top
        batcher.drawRoundedGradientRect(10, 10, 200, 28, 3.0f, 0.5f,
                                       0.25f, 0.25f, 0.28f, 1.0f,
                                       0.12f, 0.12f, 0.14f, 1.0f);
        batcher.drawText("ANALOG MIXING CONSOLE", 20, 16, 12.0f, 0.8f, 0.8f, 0.85f, 1.0f);
    }

private:
    std::shared_ptr<FluxProject> m_project;
    AudioEngine* m_engine;
    
    std::vector<std::shared_ptr<ChannelStrip>> m_channelStrips;
    std::shared_ptr<MasterStrip> m_masterStrip;
};

} // namespace Beam

#endif // MIXER_VIEW_HPP
