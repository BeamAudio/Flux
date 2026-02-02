#ifndef RENDER_PLAN_HPP
#define RENDER_PLAN_HPP

#include "engine/core/flux_node.hpp"
#include "engine/nodes/analog_base.hpp"
#include <vector>
#include <memory>
#include <atomic>

namespace Beam {

class AutomationLane;
class Parameter;

// Represents a single point-to-point signal sum operation (for signal merging)
struct SignalRoute {
    int sourceBufferIdx;
    int destBufferIdx;
};

// Represents the execution of a single processor and its subsequent data routing
struct ProcessorExecution {
    FluxProcessor* processor; 
    std::vector<const std::atomic<float>*> parameterPointers;
    
    // Buffer indices into RenderPlan::bufferPool
    std::vector<int> inputBufferIndices;
    std::vector<bool> inputConnected;
    std::vector<int> outputBufferIndices;

    std::vector<SignalRoute> outgoingRoutes;
};

// The complete immutable plan for one audio callback
struct RenderPlan {
    std::vector<ProcessorExecution> sequence;
    std::vector<std::unique_ptr<FluxProcessor>> processors; // Plan owns the processors
    // Final sink indices for the engine to copy to hardware
    std::vector<int> masterOutputBufferIndices;

    std::vector<size_t> automationLanes; // Placeholder for now
    
    // Centrally managed buffers to avoid node-local allocations
    std::vector<std::vector<float>> bufferPool;
    
    // Cached clear operations on specific buffers
    std::vector<int> clearBufferIndices;
    
    // Nodes for transport notification (on audio thread)
    std::vector<std::shared_ptr<FluxNode>> allNodes;
};

} // namespace Beam

#endif // RENDER_PLAN_HPP






