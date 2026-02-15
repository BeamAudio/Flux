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
    
    // Optional per-route gain (for mixer functionality)
    // If nullptr, gain is 1.0. Points to atomic for real-time safe control.
    const std::atomic<float>* gainPtr = nullptr;
    const std::atomic<float>* panPtr = nullptr;
    const std::atomic<bool>* mutePtr = nullptr;
    const std::atomic<bool>* soloPtr = nullptr;
    
    // Optional peak level output (for metering)
    // Audio thread writes peak level here during routing
    std::atomic<float>* peakPtr = nullptr;
};

// Represents the execution of a single processor and its subsequent data routing
struct ProcessorExecution {
    FluxProcessor* processor; 
    std::vector<const std::atomic<float>*> parameterPointers;
    std::vector<Parameter*> parameters; // Direct pointers for ramp preparation
    
    // Buffer indices into RenderPlan::bufferPool
    std::vector<int> inputBufferIndices;
    std::vector<bool> inputConnected;
    std::vector<int> outputBufferIndices;

    std::vector<SignalRoute> outgoingRoutes;
    
    // Bypass state pointer - points to FluxNode::m_bypassed for real-time safe bypass check
    const std::atomic<bool>* bypassPtr = nullptr;
};

// The complete immutable plan for one audio callback
struct RenderPlan {
    int blockSize = 0;
    int channels = 2;

    // Groups of processors that can be executed in parallel.
    // Each outer vector represents a "Level" (Wave) of execution.
    // All processors within a level are independent.
    std::vector<std::vector<ProcessorExecution>> levels;

    std::vector<std::shared_ptr<FluxProcessor>> processors; // Plan owns the processors
    // Final sink indices for the engine to copy to hardware
    std::vector<int> masterOutputBufferIndices;

    // Snapshot of active automation lanes for lock-free processing
    std::vector<std::shared_ptr<AutomationLane>> activeAutomationLanes;
    
    // Centrally managed buffers to avoid node-local allocations
    std::vector<std::vector<float>> bufferPool;
    
    // Cached clear operations on specific buffers
    std::vector<int> clearBufferIndices;
    
    // Nodes for transport notification (on audio thread)
    std::vector<std::shared_ptr<FluxNode>> allNodes;
};

} // namespace Beam

#endif // RENDER_PLAN_HPP






