#ifndef PARAMETER_QUEUE_HPP
#define PARAMETER_QUEUE_HPP

#include <vector>
#include <mutex>
#include <functional>
#include "engine/dsp/lock_free_buffer.hpp"

namespace Beam {

class Parameter;

struct ParameterMessage {
    Parameter* parameter;
    float newValue;
};

/**
 * @class ParameterQueue
 * @brief Lock-free queue for deferring parameter callbacks from Audio Thread to UI Thread.
 * Uses a Single-Producer, Single-Consumer model (Audio -> UI).
 */
class ParameterQueue {
public:
    static ParameterQueue& get() {
        static ParameterQueue instance;
        return instance;
    }

    ParameterQueue() : m_queue(1024) {}

    /**
     * @brief Pushes a message. Should be called from the Audio thread or UI thread.
     * Note: If called from multiple threads, this implementation needs a mutex for the producer side.
     * However, in BeamEngine, only one thread typically updates a specific parameter set at a time.
     */
    void push(Parameter* p, float val) {
        ParameterMessage msg{p, val};
        // If multiple threads push, we still need a mutex for the write side.
        // For P0, we assume SPSC or use a light mutex for multi-producer.
        std::lock_guard<std::mutex> lock(m_writeMutex);
        m_queue.write(&msg, 1);
    }

    /**
     * @brief Processes all pending messages on the current thread.
     * Should be called from the UI thread.
     */
    void dispatch();

private:
    std::mutex m_writeMutex; // Still needed for Multi-Producer safety if UI also calls setValue
    LockFreeBuffer<ParameterMessage> m_queue;
};

} // namespace Beam

#endif
