#ifndef RENDER_PLAN_HPP
#define RENDER_PLAN_HPP

#include "flux_node.hpp"
#include <vector>
#include <memory>
#include <atomic>

namespace Beam {

// Represents a single point-to-point signal copy operation
struct SignalRoute {
    float* sourceBuffer;
    float* destBuffer;
    // We assume buffer size is constant per block (e.g. 1024 frames * 2 channels)
};

// Represents the execution of a single node and its subsequent data routing
struct NodeExecution {
    std::shared_ptr<FluxNode> node; // Keep alive while plan exists
    std::vector<SignalRoute> outgoingRoutes;
};

// The complete immutable plan for one audio callback
struct RenderPlan {
    std::vector<NodeExecution> sequence;
    
    // Cached clear operations (inputs that need to be zeroed before processing)
    struct BufferClearOp {
        float* buffer;
        size_t size;
    };
    std::vector<BufferClearOp> clearOps;
};

} // namespace Beam

#endif // RENDER_PLAN_HPP
