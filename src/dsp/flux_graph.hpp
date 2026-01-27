#ifndef FLUX_GRAPH_HPP
#define FLUX_GRAPH_HPP

#include "flux_node.hpp"
#include <map>
#include <set>
#include <algorithm>

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
        size_t id = m_nextId++;
        m_nodes[id] = node;
        m_needsRebuild = true;
        return id;
    }

    void removeNode(size_t id) {
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
        m_connections.insert({srcNodeId, srcPort, dstNodeId, dstPort});
        m_needsRebuild = true;
    }

    void disconnect(size_t srcNodeId, int srcPort, size_t dstNodeId, int dstPort) {
        m_connections.erase({srcNodeId, srcPort, dstNodeId, dstPort});
        m_needsRebuild = true;
    }

    void process(int frames) {
        if (m_needsRebuild) rebuildSchedule();

        // 1. Clear input buffers of all nodes
        for (auto& pair : m_nodes) {
            auto node = pair.second;
            for (int i = 0; i < node->getInputPorts().size(); ++i) {
                float* buf = node->getInputBuffer(i);
                std::fill(buf, buf + frames * 2, 0.0f); // Assuming stereo for now
            }
        }

        // 2. Process nodes in topological order
        for (size_t nodeId : m_schedule) {
            auto node = m_nodes[nodeId];
            if (!node->isBypassed()) {
                node->process(frames);
            } else {
                // If bypassed, we might want to pass through if it's an FX node
                // For now, just clear outputs
                for (int i = 0; i < node->getOutputPorts().size(); ++i) {
                    float* buf = node->getOutputBuffer(i);
                    std::fill(buf, buf + frames * 2, 0.0f);
                }
            }

            // 3. Propagate outputs to connected inputs
            for (const auto& conn : m_connections) {
                if (conn.srcNodeId == nodeId) {
                    auto srcNode = m_nodes[conn.srcNodeId];
                    auto dstNode = m_nodes[conn.dstNodeId];
                    float* srcBuf = srcNode->getOutputBuffer(conn.srcPortIdx);
                    float* dstBuf = dstNode->getInputBuffer(conn.dstPortIdx);
                    
                    for (int i = 0; i < frames * 2; ++i) {
                        dstBuf[i] += srcBuf[i];
                    }
                }
            }
        }
    }

    std::shared_ptr<FluxNode> getNode(size_t id) { return m_nodes[id]; }
    const std::map<size_t, std::shared_ptr<FluxNode>>& getNodes() const { return m_nodes; }

private:
    void rebuildSchedule() {
        m_schedule.clear();
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
            m_schedule.push_back(u);

            for (const auto& conn : m_connections) {
                if (conn.srcNodeId == u) {
                    inDegree[conn.dstNodeId]--;
                    if (inDegree[conn.dstNodeId] == 0) {
                        queue.push_back(conn.dstNodeId);
                    }
                }
            }
        }
        m_needsRebuild = false;
    }

    std::map<size_t, std::shared_ptr<FluxNode>> m_nodes;
    std::set<FluxConnection> m_connections;
    std::vector<size_t> m_schedule;
    size_t m_nextId = 0;
    bool m_needsRebuild = true;
};

} // namespace Beam

#endif // FLUX_GRAPH_HPP
