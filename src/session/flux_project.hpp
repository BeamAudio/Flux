#ifndef FLUX_PROJECT_HPP
#define FLUX_PROJECT_HPP

#include "../engine/flux_graph.hpp"
#include "../engine/flux_track_node.hpp"
#include "region.hpp"
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
};

class FluxProject {
public:
    FluxProject() {
        m_graph = std::make_shared<FluxGraph>();
    }

    std::shared_ptr<FluxGraph> getGraph() { return m_graph; }
    
    void addTrack(TrackData td) {
        m_tracks.push_back(td);
    }

    std::vector<TrackData>& getTracks() { return m_tracks; }

    nlohmann::json serialize() const {
        nlohmann::json data;
        data["name"] = m_name;
        return data;
    }

    void deserialize(const nlohmann::json& data) {
        if (data.contains("name")) m_name = data["name"];
    }

private:
    std::string m_name = "Untitled Flux";
    std::shared_ptr<FluxGraph> m_graph;
    std::vector<TrackData> m_tracks;
};

} // namespace Beam

#endif // FLUX_PROJECT_HPP
