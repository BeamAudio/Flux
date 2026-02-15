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

        // --- DEPTH CALCULATION (for Parallelism) ---
        std::map<size_t, int> nodeDepth;
        int maxDepth = 0;
        for (size_t nodeId : schedule) {
            int depth = 0;
            for (const auto& conn : m_connections) {
                if (conn.dstNodeId == nodeId) {
                    depth = (std::max)(depth, nodeDepth[conn.srcNodeId] + 1);
                }
            }
            nodeDepth[nodeId] = depth;
            maxDepth = (std::max)(maxDepth, depth);
        }
        plan->levels.resize(maxDepth + 1);
        // --------------------------------------------

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

        // 2. Instantiate Processors
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

        // 3. Merged Buffer Allocation & Execution Build (NO REUSE for Maximum Stability)
        std::map<size_t, std::vector<int>> nodeInputBuffers; 
        std::map<size_t, std::vector<int>> nodeOutputBuffers;

        auto getBuffer = [&]() -> int {
            int idx = (int)plan->bufferPool.size();
            plan->bufferPool.push_back(std::vector<float>(bufferSizeFrames * channels, 0.0f));
            plan->clearBufferIndices.push_back(idx); 
            return idx;
        };

        for (size_t nodeId : schedule) {
            auto node = nodeLookup[nodeId];
            
            // Assign unique buffers for all inputs
            for (int i = 0; i < (int)node->getInputPorts().size(); ++i) {
                nodeInputBuffers[nodeId].push_back(getBuffer());
            }
            
            // Assign unique buffers for all outputs
            for (int i = 0; i < (int)node->getOutputPorts().size(); ++i) {
                nodeOutputBuffers[nodeId].push_back(getBuffer());
            }
        }

        for (size_t nodeId : schedule) {
            auto node = nodeLookup[nodeId];
            auto processor = nodeToProcessor[nodeId];
            if (!processor) continue;

            ProcessorExecution exec;
            exec.processor = processor;
            exec.inputBufferIndices = nodeInputBuffers[nodeId];
            exec.outputBufferIndices = nodeOutputBuffers[nodeId];

            // Master Output Tracking
            if (nodeId == masterNodeId) {
                plan->masterOutputBufferIndices = exec.outputBufferIndices;
            }

            // C. Setup Processor Params & Connections
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
                exec.parameters.push_back(param.get()); 
            }
            
            // Set bypass pointer for real-time safe bypass checking
            exec.bypassPtr = node->getBypassAtomic();

            // D. Build Routes & PDC
            std::vector<ProcessorExecution> pdcExecs;
            for (const auto& conn : m_connections) {
                if (conn.srcNodeId == nodeId) {
                    if (conn.srcPortIdx >= (int)nodeOutputBuffers[nodeId].size()) continue;
                    
                    int srcBufIdx = nodeOutputBuffers[nodeId][conn.srcPortIdx];
                    int dstBufIdx = nodeInputBuffers[conn.dstNodeId][conn.dstPortIdx];

                    SignalRoute route;
                    size_t srcLat = nodeTotalLatency[nodeId];
                    size_t targetLat = maxInLatencyAtNode[conn.dstNodeId];

                    if (srcLat < targetLat) {
                        size_t diff = targetLat - srcLat;
                        auto pdcProc = std::make_shared<PDCDelayProcessor>(diff, bufferSizeFrames);
                        FluxProcessor* pdcPtr = pdcProc.get();
                        plan->processors.push_back(std::move(pdcProc));

                        int intermediateBuf = getBuffer();
                        
                        route.sourceBufferIdx = srcBufIdx;
                        route.destBufferIdx = intermediateBuf;
                        if (conn.dstNodeId == masterNodeId && mixerState) {
                             MixChannel* ch = mixerState->getOrCreateChannel(nodeId);
                             if (ch) {
                                 route.gainPtr = &ch->gain;
                                 route.panPtr = &ch->pan;
                                 route.mutePtr = &ch->muted;
                                 route.soloPtr = &ch->solo;
                                 route.peakPtr = &ch->peakL;
                             }
                        }
                        exec.outgoingRoutes.push_back(route);

                        ProcessorExecution pdcExec;
                        pdcExec.processor = pdcPtr;
                        pdcExec.inputBufferIndices = {intermediateBuf};
                        pdcExec.outputBufferIndices = {dstBufIdx}; 
                        pdcExec.inputConnected = {true};

                        pdcExecs.push_back(pdcExec);
                    } else {
                        route.sourceBufferIdx = srcBufIdx;
                        route.destBufferIdx = dstBufIdx;
                        if (conn.dstNodeId == masterNodeId && mixerState) {
                             MixChannel* ch = mixerState->getOrCreateChannel(nodeId);
                             if (ch) {
                                 route.gainPtr = &ch->gain;
                                 route.panPtr = &ch->pan;
                                 route.mutePtr = &ch->muted;
                                 route.soloPtr = &ch->solo;
                                 route.peakPtr = &ch->peakL;
                             }
                        }
                        exec.outgoingRoutes.push_back(route);
                    }
                }
            }

            int depth = nodeDepth[nodeId];
            plan->levels[depth].push_back(exec);
            
            if (!pdcExecs.empty()) {
                if (plan->levels.size() <= (size_t)depth + 1) plan->levels.resize(depth + 2);
                for (auto& p : pdcExecs) plan->levels[depth + 1].push_back(p);
            }
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






