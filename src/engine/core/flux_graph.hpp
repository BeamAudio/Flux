#ifndef FLUX_GRAPH_HPP
#define FLUX_GRAPH_HPP

#include "engine/core/flux_node.hpp"
#include "engine/core/render_plan.hpp"
#include "engine/nodes/pdc_delay_node.hpp"
#include "engine/session/mixer_state.hpp"
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

    size_t reserveNextId() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_nextId++;
    }

    void addNodeWithId(std::shared_ptr<FluxNode> node, size_t id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_nodes[id] = node;
        if (id >= m_nextId) m_nextId = id + 1;
        m_needsRebuild = true;
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

#include "json.hpp"

    // Serialization
    nlohmann::json serialize() {
        std::lock_guard<std::mutex> lock(m_mutex);
        nlohmann::json data;
        
        // Nodes
        nlohmann::json nodes = nlohmann::json::array();
        for (const auto& [id, node] : m_nodes) {
            nlohmann::json n = node->serialize();
            n["id"] = id;
            nodes.push_back(n);
        }
        data["nodes"] = nodes;

        // Connections
        nlohmann::json conns = nlohmann::json::array();
        for (const auto& c : m_connections) {
            conns.push_back({
                {"srcId", c.srcNodeId},
                {"srcPort", c.srcPortIdx},
                {"dstId", c.dstNodeId},
                {"dstPort", c.dstPortIdx}
            });
        }
        data["connections"] = conns;
        
        // Save ID counter
        data["nextId"] = m_nextId;
        std::cout << "[Serialize] Graph has " << nodes.size() << " nodes and " << conns.size() << " connections." << std::endl;
        return data;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_nodes.clear();
        m_connections.clear();
        m_nextId = 0;
        m_needsRebuild = true;
    }

    // Compiles the current graph topology into an optimized, flat execution plan.
    // This version implements Plugin Delay Compensation (PDC).
    std::shared_ptr<RenderPlan> compile(int bufferSizeFrames, int channels = 2, size_t masterNodeId = (size_t)-1, MixerState* mixerState = nullptr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto plan = std::make_shared<RenderPlan>();
        plan->blockSize = bufferSizeFrames;
        plan->channels = channels;
        
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

        // --- PDC CALCULATION ---
        std::map<size_t, size_t> nodeTotalLatency;
        std::map<size_t, std::map<int, size_t>> portLatency; // nodeId -> portIdx -> latency
        std::map<size_t, size_t> maxInLatencyAtNode;

        for (size_t nodeId : schedule) {
            auto node = m_nodes[nodeId];
            size_t maxIn = 0;
            for (auto const& [port, lat] : portLatency[nodeId]) {
                if (lat > maxIn) maxIn = lat;
            }
            maxInLatencyAtNode[nodeId] = maxIn;
            nodeTotalLatency[nodeId] = maxIn + node->getLatency();
            
            for (const auto& conn : m_connections) {
                if (conn.srcNodeId == nodeId) {
                    portLatency[conn.dstNodeId][conn.dstPortIdx] = nodeTotalLatency[nodeId];
                }
            }
        }
        // -----------------------

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
            
            exec.inputConnected.assign(exec.inputBufferIndices.size(), false);
            for (const auto& conn : m_connections) {
                if (conn.dstNodeId == nodeId) {
                    if (conn.dstPortIdx < (int)exec.inputConnected.size()) {
                        exec.inputConnected[conn.dstPortIdx] = true;
                    }
                }
            }

            for (auto const& param : node->getParameterOrder()) {
                exec.parameterPointers.push_back(param->getTargetValueAtomic());
            }

            // Routes: Push current node's outputs to downstream inputs
            std::vector<ProcessorExecution> pdcExecs;
            for (const auto& conn : m_connections) {
                if (conn.srcNodeId == nodeId) {
                    if (conn.srcPortIdx >= (int)nodeOutputBuffers[nodeId].size() || 
                        conn.dstPortIdx >= (int)nodeInputBuffers[conn.dstNodeId].size()) {
                        std::cout << "[Compile] Error: Invalid port connection " << conn.srcPortIdx << " -> " << conn.dstPortIdx << std::endl;
                        continue;
                    }

                    SignalRoute route;
                    int srcBufIdx = nodeOutputBuffers[nodeId][conn.srcPortIdx];
                    int dstBufIdx = nodeInputBuffers[conn.dstNodeId][conn.dstPortIdx];

                    // PDC Compensation check
                    size_t srcLat = nodeTotalLatency[nodeId];
                    size_t targetLat = maxInLatencyAtNode[conn.dstNodeId];

                    if (srcLat < targetLat) {
                        size_t diff = targetLat - srcLat;
                        auto pdcProc = std::make_shared<PDCDelayProcessor>(diff, bufferSizeFrames);
                        FluxProcessor* pdcPtr = pdcProc.get();
                        plan->processors.push_back(std::move(pdcProc));

                        int intermediateBuf = getBuffer();
                        plan->clearBufferIndices.push_back(intermediateBuf);

                        // Route: Node Output -> Intermediate
                        route.sourceBufferIdx = srcBufIdx;
                        route.destBufferIdx = intermediateBuf;
                        
                        if (conn.dstNodeId == masterNodeId && mixerState) {
                            MixChannel* ch = mixerState->getOrCreateChannel(nodeId);
                            if (ch) {
                                route.gainPtr = &ch->gain;
                                route.mutePtr = &ch->muted;
                                route.peakPtr = &ch->peakL;
                            }
                        }
                        exec.outgoingRoutes.push_back(route);

                        // PDC step
                        ProcessorExecution pdcExec;
                        pdcExec.processor = pdcPtr;
                        pdcExec.inputBufferIndices = {intermediateBuf};
                        pdcExec.outputBufferIndices = {dstBufIdx};
                        pdcExec.inputConnected = {true};
                        pdcExecs.push_back(pdcExec);
                    } else {
                        // Normal Route
                        route.sourceBufferIdx = srcBufIdx;
                        route.destBufferIdx = dstBufIdx;
                        
                        if (conn.dstNodeId == masterNodeId && mixerState) {
                            MixChannel* ch = mixerState->getOrCreateChannel(nodeId);
                            if (ch) {
                                route.gainPtr = &ch->gain;
                                route.mutePtr = &ch->muted;
                                route.peakPtr = &ch->peakL;
                            }
                        }
                        exec.outgoingRoutes.push_back(route); 
                    }
                }
            }
            // CRITICAL FIX: Push the original node execution ONCE
            plan->sequence.push_back(exec);
            // Then push its associated PDC delay steps
            for (auto& p : pdcExecs) plan->sequence.push_back(p);
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






