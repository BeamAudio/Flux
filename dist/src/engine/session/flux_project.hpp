#ifndef FLUX_PROJECT_HPP
#define FLUX_PROJECT_HPP

#include "engine/core/flux_graph.hpp"
#include "engine/nodes/flux_track_node.hpp"
#include "engine/scripting/flux_script_node.hpp"
#include "engine/plugins/plugin_registry.hpp"
#include "engine/session/region.hpp"
#include "engine/session/automation.hpp"
#include <string>
#include <memory>
#include <vector>
#include "json.hpp"

namespace Beam {

/**
 * @struct TrackData
 * @brief Represents a single track which has a DSP node and multiple timeline regions.
 */
struct TrackData {
    std::shared_ptr<FluxTrackNode> node;
    size_t nodeId;
    std::vector<Region> regions;
    int trackIndex;
    std::vector<std::shared_ptr<AutomationLane>> automationLanes; 
};

class FluxProject {
public:
    FluxProject() {
        m_graph = std::make_shared<FluxGraph>();
    }

    std::shared_ptr<FluxGraph> getGraph() { return m_graph; }
    
    void addTrack(TrackData td) {
        m_tracks.push_back(td);
        setDirty(true);
    }

    std::vector<TrackData>& getTracks() { return m_tracks; }

    void setDirty(bool dirty) { m_isDirty = dirty; }
    bool isDirty() const { return m_isDirty; }

    nlohmann::json serialize() const {
        m_isDirty = false; // System is clean after save
        nlohmann::json data;
        data["name"] = m_name;
        data["graph"] = m_graph->serialize();
        
        nlohmann::json trackData = nlohmann::json::array();
        for (const auto& t : m_tracks) {
            nlohmann::json td;
            td["nodeId"] = t.nodeId;
            td["trackIndex"] = t.trackIndex;
            
            nlohmann::json regions = nlohmann::json::array();
            for (const auto& r : t.regions) {
                regions.push_back({
                    {"name", r.name},
                    {"start", r.startFrame},
                    {"duration", r.duration},
                    {"offset", r.sourceOffset},
                    {"trackIdx", r.trackIndex}
                });
            }
            td["regions"] = regions;
            
            // Automation
            nlohmann::json autos = nlohmann::json::array();
            for (const auto& lane : t.automationLanes) {
                nlohmann::json l;
                l["paramName"] = lane->getParameter()->getName();
                nlohmann::json pts = nlohmann::json::array();
                for(auto& p : lane->getPoints()) {
                    pts.push_back({{"f", p.frame}, {"v", p.value}});
                }
                l["points"] = pts;
                autos.push_back(l);
            }
            td["automation"] = autos;

            trackData.push_back(td);
        }
        data["tracks"] = trackData;
        data["visuals"] = m_visuals; // Workspace will populate this before save
        
        return data;
    }

    void deserialize(const nlohmann::json& data) {
        if (data.contains("name")) m_name = data["name"];
        m_visuals.clear();
        if (data.contains("visuals")) m_visuals = data["visuals"]; // Restore map

        // Rebuild Graph
        m_graph->clear();
        m_tracks.clear();
        

        
        if (data.contains("graph")) {
            auto& gData = data["graph"];
            
            // 1. Restore Nodes
            for (const auto& nData : gData["nodes"]) {
                size_t id = nData["id"];
                std::string name = nData["name"];
                std::string type = nData.value("type", "Effect"); 
                
                std::shared_ptr<FluxNode> newNode;
                
                if (type == "FluxTrackNode") {
                     std::string path = nData.value("audioFilePath", "");
                     auto track = std::make_shared<FluxTrackNode>(name, 4096);
                     if (!path.empty()) track->load(path);
                     newNode = track;
                } else if (type == "FluxScriptNode") {
                     std::string scriptPath = nData.value("scriptPath", "");
                     if (!scriptPath.empty()) {
                         newNode = std::make_shared<FluxScriptNode>(scriptPath, 4096, 44100.0f);
                     }
                } else if (name == "Master") {
                     newNode = std::make_shared<MasterNode>(4096);
                } else {
                    newNode = PluginRegistry::get().createPlugin(name, 4096, 44100);
                }
                
                if (newNode) {
                    newNode->deserialize(nData);
                    m_graph->addNodeWithId(newNode, id);
                    std::cout << "[Deserialize] Restored Node: " << name << " ID: " << id << std::endl;
                } else {
                    std::cout << "[Deserialize] Warning: Could not restore node type: " << name << std::endl;
                }
            }
            std::cout << "[Deserialize] Total Nodes Restored: " << m_graph->getNodes().size() << std::endl;
            
            // 2. Restore Connections (Direct ID usage)
            for (const auto& cData : gData["connections"]) {
                m_graph->connect(cData["srcId"], cData["srcPort"], cData["dstId"], cData["dstPort"]);
            }
        }
        
        // Restore Tracks Metadata
        if (data.contains("tracks")) {
            for (const auto& tData : data["tracks"]) {
                size_t nodeId = tData["nodeId"];
                // Verify node exists
                if (!m_graph->getNode(nodeId)) continue;

                TrackData td;
                td.nodeId = nodeId;
                td.node = std::dynamic_pointer_cast<FluxTrackNode>(m_graph->getNode(td.nodeId));
                td.trackIndex = tData["trackIndex"];
                
                // Regions
                for (const auto& rData : tData["regions"]) {
                    Region r;
                    r.name = rData["name"];
                    r.startFrame = rData["start"];
                    r.duration = rData["duration"];
                    r.sourceOffset = rData["offset"];
                    r.trackIndex = rData["trackIdx"];
                    // Peak data generation would happen here (async) or we rely on TrackNode to reload file
                    // If TrackNode loaded file, it has peaks.
                    if (td.node) r.channelPeaks = td.node->getPeakData(400); 
                    td.regions.push_back(r);
                }

                // Automation
                if (tData.contains("automation") && td.node) {
                     for (const auto& lData : tData["automation"]) {
                         std::string pName = lData["paramName"];
                         if (auto p = td.node->getParameter(pName)) {
                             auto lane = std::make_shared<AutomationLane>(p);
                             for (const auto& pt : lData["points"]) {
                                 lane->addPoint(pt["f"], pt["v"]);
                             }
                             td.automationLanes.push_back(lane);
                         }
                     }
                }

                m_tracks.push_back(td);
            }
        }

        // Remap Visuals
        if (data.contains("visuals")) {
             m_visuals = data["visuals"];
        }
    }

    // Visual State Storage
    mutable nlohmann::json m_visuals; 
    
    void setVisualPos(size_t nodeId, float x, float y) {
        m_visuals[std::to_string(nodeId)] = {{"x", x}, {"y", y}};
    }
    
    std::pair<float, float> getVisualPos(size_t nodeId) const {
        std::string key = std::to_string(nodeId);
        if (m_visuals.contains(key)) {
            std::cout << "[GetVisualPos] Found ID " << nodeId << " at (" << m_visuals[key]["x"].get<float>() << ", " << m_visuals[key]["y"].get<float>() << ")" << std::endl;
            return {m_visuals[key]["x"].get<float>(), m_visuals[key]["y"].get<float>()};
        }
        std::cout << "[GetVisualPos] ID " << nodeId << " NOT FOUND in visuals. Returning default." << std::endl;
        return {100.0f, 100.0f};
    }

private:
    mutable bool m_isDirty = false;
    std::string m_name = "Untitled Flux";
    std::shared_ptr<FluxGraph> m_graph;
    std::vector<TrackData> m_tracks;
};

} // namespace Beam

#endif // FLUX_PROJECT_HPP





