#ifndef PARAMETER_QUEUE_HPP
#define PARAMETER_QUEUE_HPP

#include <vector>
#include <mutex>
#include <functional>

namespace Beam {

class Parameter;

struct ParameterMessage {
    Parameter* parameter;
    float newValue;
};

/**
 * @class ParameterQueue
 * @brief Thread-safe queue for deferring parameter callbacks from Audio Thread to UI Thread.
 */
class ParameterQueue {
public:
    static ParameterQueue& get() {
        static ParameterQueue instance;
        return instance;
    }

    void push(Parameter* p, float val) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_messages.push_back({p, val});
    }

    /**
     * @brief Processes all pending messages on the current thread.
     * Should be called from the UI thread.
     */
    void dispatch();

private:
    std::mutex m_mutex;
    std::vector<ParameterMessage> m_messages;
};

} // namespace Beam

#endif
