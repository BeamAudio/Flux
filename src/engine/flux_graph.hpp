#ifndef FLUX_GRAPH_HPP
#define FLUX_GRAPH_HPP

#include "flux_node.hpp"
#include "render_plan.hpp"
#include <map>
#include <set>
#include <algorithm>
#include <iostream>
#include <mutex>

namespace Beam {

struct FluxConnection {
    size_t srcNodeId;
    int srcPortIdx;
    size_t dstNodeId;
    int dstPortIdx;

    bool operator<(const FluxConnection& other) const {
        if (srcNodeId != other.srcNodeId) return srcNodeId < other.srcNodeId;
        if (srcPortIdx != other.srcPortIdx) return srcPortIdx < other.srcPortIdx;
        if (dstNodeId != other.dstNodeId) return dstNodeId < other.dstNodeId;
        return dstPortIdx < other.dstPortIdx;
    }
};

class FluxGraph {
public:
    size_t addNode(std::shared_ptr<FluxNode> node) {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t id = m_nextId++;
        m_nodes[id] = node;
        m_needsRebuild = true;
        return id;
    }

    void removeNode(size_t id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_nodes.erase(id);
        // Remove associated connections
        for (auto it = m_connections.begin(); it != m_connections.end(); ) {
            if (it->srcNodeId == id || it->dstNodeId == id) {
                it = m_connections.erase(it);
            } else {
                ++it;
            }
        }
        m_needsRebuild = true;
    }

    void connect(size_t srcNodeId, int srcPort, size_t dstNodeId, int dstPort) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_connections.insert({srcNodeId, srcPort, dstNodeId, dstPort});
        m_needsRebuild = true;
    }

    void disconnect(size_t srcNodeId, int srcPort, size_t dstNodeId, int dstPort) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_connections.erase({srcNodeId, srcPort, dstNodeId, dstPort});
        m_needsRebuild = true;
    }

    // Compiles the current graph topology into an optimized, flat execution plan.
    // This method is O(N + C) and should be called from the UI thread when the graph changes.
    std::shared_ptr<RenderPlan> compile(int bufferSizeFrames, int channels = 2) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto plan = std::make_shared<RenderPlan>();
        
        // 1. Topological Sort
        std::vector<size_t> schedule;
        std::map<size_t, int> inDegree;
        for (auto const& [id, node] : m_nodes) inDegree[id] = 0;

        for (const auto& conn : m_connections) {
            inDegree[conn.dstNodeId]++;
        }

        std::vector<size_t> queue;
        for (auto const& [id, degree] : inDegree) {
            if (degree == 0) queue.push_back(id);
        }

        while (!queue.empty()) {
            size_t u = queue.back();
            queue.pop_back();
            schedule.push_back(u);

            for (const auto& conn : m_connections) {
                if (conn.srcNodeId == u) {
                    inDegree[conn.dstNodeId]--;
                    if (inDegree[conn.dstNodeId] == 0) {
                        queue.push_back(conn.dstNodeId);
                    }
                }
            }
        }

        // 2. Build Execution Plan
        size_t bufSizeFloats = bufferSizeFrames * channels;

        // Identify all input buffers that need clearing
        for (const auto& [id, node] : m_nodes) {
            for (int i = 0; i < (int)node->getInputPorts().size(); ++i) {
                plan->clearOps.push_back({ node, i });
            }
        }

        // Map ID to Node Ptr for quick lookup
        std::map<size_t, std::shared_ptr<FluxNode>> nodeLookup = m_nodes;

        for (size_t nodeId : schedule) {
            auto node = nodeLookup[nodeId];
            NodeExecution exec;
            exec.node = node;

            // Pre-calculate routing for this node's outputs
            for (const auto& conn : m_connections) {
                if (conn.srcNodeId == nodeId) {
                    auto dstNode = nodeLookup[conn.dstNodeId];
                    if (dstNode) {
                        exec.outgoingRoutes.push_back({
                            node,
                            conn.srcPortIdx,
                            dstNode,
                            conn.dstPortIdx
                        });
                    }
                }
            }
            plan->sequence.push_back(exec);
        }

        m_needsRebuild = false;
        return plan;
    }

    void setTransportState(bool playing) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& pair : m_nodes) {
            pair.second->onTransportStateChanged(playing);
        }
    }

    std::shared_ptr<FluxNode> getNode(size_t id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_nodes.find(id);
        return (it != m_nodes.end()) ? it->second : nullptr;
    }

    const std::map<size_t, std::shared_ptr<FluxNode>>& getNodes() const {
        return m_nodes;
    }

    const std::set<FluxConnection>& getConnections() const {
        return m_connections;
    }

private:
    std::map<size_t, std::shared_ptr<FluxNode>> m_nodes;
    std::set<FluxConnection> m_connections;
    size_t m_nextId = 0;
    bool m_needsRebuild = true;
    std::mutex m_mutex;
};

} // namespace Beam

#endif // FLUX_GRAPH_HPP

