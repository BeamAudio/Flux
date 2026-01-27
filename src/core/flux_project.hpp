#ifndef FLUX_PROJECT_HPP
#define FLUX_PROJECT_HPP

#include "../dsp/flux_graph.hpp"
#include <string>
#include <memory>
#include "json.hpp"

namespace Beam {

class FluxProject {
public:
    FluxProject() {
        m_graph = std::make_shared<FluxGraph>();
    }

    std::shared_ptr<FluxGraph> getGraph() { return m_graph; }

    nlohmann::json serialize() const {
        nlohmann::json data;
        data["name"] = m_name;
        // Serialize graph, modules, etc.
        return data;
    }

    void deserialize(const nlohmann::json& data) {
        if (data.contains("name")) m_name = data["name"];
    }

private:
    std::string m_name = "Untitled Flux";
    std::shared_ptr<FluxGraph> m_graph;
};

} // namespace Beam

#endif // FLUX_PROJECT_HPP
