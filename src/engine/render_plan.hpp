#ifndef RENDER_PLAN_HPP
#define RENDER_PLAN_HPP

#include "flux_node.hpp"
#include "analog_base.hpp"
#include <vector>
#include <memory>
#include <atomic>

namespace Beam {

// Represents a single point-to-point signal copy operation
struct SignalRoute {
    std::shared_ptr<FluxNode> sourceNode;
    int sourcePort;
    std::shared_ptr<FluxNode> destNode;
    int destPort;
};

// Represents the execution of a single node and its subsequent data routing
struct NodeExecution {
    std::shared_ptr<FluxNode> node; 
    std::vector<SignalRoute> outgoingRoutes;
};

// The complete immutable plan for one audio callback
struct RenderPlan {
    std::vector<NodeExecution> sequence;
    
    // Cached clear operations (inputs that need to be zeroed before processing)
    struct BufferClearOp {
        std::shared_ptr<FluxNode> node;
        int portIdx;
    };
    std::vector<BufferClearOp> clearOps;
};

} // namespace Beam

#endif // RENDER_PLAN_HPP






