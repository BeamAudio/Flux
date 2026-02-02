#ifndef FLUX_GRAPH_HPP
#define FLUX_GRAPH_HPP

#include "engine/core/flux_node.hpp"
#include "engine/core/render_plan.hpp"
#include "engine/nodes/pdc_delay_node.hpp"
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
    // This version implements Plugin Delay Compensation (PDC).
    std::shared_ptr<RenderPlan> compile(int bufferSizeFrames, int channels = 2, size_t masterNodeId = (size_t)-1) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto plan = std::make_shared<RenderPlan>();
        
        // 1. Topological Sort
        std::vector<size_t> schedule;
        std::map<size_t, int> inDegree;
        for (auto const& [id, node] : m_nodes) inDegree[id] = 0;
        for (const auto& conn : m_connections) inDegree[conn.dstNodeId]++;

        std::vector<size_t> queue;
        for (auto const& [id, degree] : inDegree) if (degree == 0) queue.push_back(id);

        while (!queue.empty()) {
            size_t u = queue.back();
            queue.pop_back();
            schedule.push_back(u);
            for (const auto& conn : m_connections) {
                if (conn.srcNodeId == u) {
                    inDegree[conn.dstNodeId]--;
                    if (inDegree[conn.dstNodeId] == 0) queue.push_back(conn.dstNodeId);
                }
            }
        }

        // 2. Instantiate Processors and Capture Parameters
        std::map<size_t, FluxProcessor*> nodeToProcessor;
        std::map<size_t, std::shared_ptr<FluxNode>> nodeLookup = m_nodes;

        for (size_t nodeId : schedule) {
            auto node = nodeLookup[nodeId];
            auto processor = node->createProcessor();
            if (processor) {
                FluxProcessor* ptr = processor.get();
                nodeToProcessor[nodeId] = ptr;
                plan->processors.push_back(std::move(processor));
            }
        }

        // 3. Buffer Allocation & Routing
        // We assign a buffer for every output port of every node in the schedule
        // and every input port.
        struct PortRef { size_t nodeId; int portIdx; bool isOutput; };
        std::map<size_t, std::vector<int>> nodeInputBuffers;
        std::map<size_t, std::vector<int>> nodeOutputBuffers;

        int bufferCount = 0;
        auto getBuffer = [&]() {
            plan->bufferPool.push_back(std::vector<float>(bufferSizeFrames * channels, 0.0f));
            return bufferCount++;
        };

        for (size_t nodeId : schedule) {
            auto node = nodeLookup[nodeId];
            for (int i = 0; i < (int)node->getInputPorts().size(); ++i) {
                int bIdx = getBuffer();
                nodeInputBuffers[nodeId].push_back(bIdx);
                plan->clearBufferIndices.push_back(bIdx);
            }
            for (int i = 0; i < (int)node->getOutputPorts().size(); ++i) {
                int bIdx = getBuffer();
                nodeOutputBuffers[nodeId].push_back(bIdx);
                plan->clearBufferIndices.push_back(bIdx); // Also clear outputs!
            }
            
            // If this is the master node, track its input buffers (or output if it has them)
            if (nodeId == masterNodeId) {
                // The engine wants the final output after master processing.
                // Usually this is the output of the master node.
                plan->masterOutputBufferIndices = nodeOutputBuffers[nodeId];
            }
        }

        // 4. Build Execution Sequence
        for (size_t nodeId : schedule) {
            auto node = nodeLookup[nodeId];
            auto processor = nodeToProcessor[nodeId];
            if (!processor) continue;

            ProcessorExecution exec;
            exec.processor = processor;
            exec.inputBufferIndices = nodeInputBuffers[nodeId];
            exec.outputBufferIndices = nodeOutputBuffers[nodeId];
            
            // Track which inputs actually have connections
            exec.inputConnected.assign(exec.inputBufferIndices.size(), false);
            for (const auto& conn : m_connections) {
                if (conn.dstNodeId == nodeId) {
                    if (conn.dstPortIdx < (int)exec.inputConnected.size()) {
                        exec.inputConnected[conn.dstPortIdx] = true;
                    }
                }
            }

            // Snap Parameters (In preservation order)
            for (auto const& param : node->getParameterOrder()) {
                exec.parameterPointers.push_back(param->getTargetValueAtomic());
            }

            // Routes: Push current node's outputs to downstream inputs
            for (const auto& conn : m_connections) {
                if (conn.srcNodeId == nodeId) {
                    SignalRoute route;
                    route.sourceBufferIdx = nodeOutputBuffers[nodeId][conn.srcPortIdx];
                    
                    auto dstIt = nodeInputBuffers.find(conn.dstNodeId);
                    if (dstIt != nodeInputBuffers.end()) {
                        route.destBufferIdx = dstIt->second[conn.dstPortIdx];
                        exec.outgoingRoutes.push_back(route); 
                    }
                }
            }
            plan->sequence.push_back(exec);
        }

        // 5. Broadcaster Tracking (for transport events)
        for(auto const& [id, node] : m_nodes) {
            plan->allNodes.push_back(node);
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






