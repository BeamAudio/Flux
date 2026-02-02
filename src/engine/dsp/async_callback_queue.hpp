#ifndef ASYNC_CALLBACK_QUEUE_HPP
#define ASYNC_CALLBACK_QUEUE_HPP

#include <vector>
#include <mutex>
#include <functional>

namespace Beam {

/**
 * @class AsyncCallbackQueue
 * @brief Thread-safe queue for deferring arbitrary callbacks to the UI thread.
 */
class AsyncCallbackQueue {
public:
    static AsyncCallbackQueue& get() {
        static AsyncCallbackQueue instance;
        return instance;
    }

    void push(std::function<void()> callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callbacks.push_back(std::move(callback));
    }

    /**
     * @brief Processes all pending callbacks on the current thread.
     * Should be called from the UI thread (main loop).
     */
    void dispatch() {
        std::vector<std::function<void()>> localCallbacks;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_callbacks.empty()) return;
            localCallbacks.swap(m_callbacks);
        }

        for (auto& cb : localCallbacks) {
            if (cb) cb();
        }
    }

private:
    std::mutex m_mutex;
    std::vector<std::function<void()>> m_callbacks;
};

} // namespace Beam

#endif // ASYNC_CALLBACK_QUEUE_HPP
