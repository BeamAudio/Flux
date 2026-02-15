#ifndef BEAM_THREAD_POOL_HPP
#define BEAM_THREAD_POOL_HPP

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <future>

namespace Beam {

/**
 * @class ThreadPool
 * @brief Simple task-based thread pool for parallel DSP processing.
 */
class ThreadPool {
public:
    static ThreadPool& get() {
        static ThreadPool instance(std::thread::hardware_concurrency());
        return instance;
    }

    ThreadPool(size_t threads) : m_stop(false) {
        for (size_t i = 0; i < threads; ++i) {
            m_workers.emplace_back([this] {
                for (;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(m_queueMutex);
                        m_condition.wait(lock, [this] { return m_stop || !m_tasks.empty(); });
                        if (m_stop && m_tasks.empty()) return;
                        task = std::move(m_tasks.front());
                        m_tasks.pop();
                    }
                    task();
                    m_completedTasks++;
                }
            });
        }
    }

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using return_type = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            if (m_stop) throw std::runtime_error("enqueue on stopped ThreadPool");
            m_tasks.emplace([task]() { (*task)(); });
        }
        m_condition.notify_one();
        return res;
    }

    /**
     * @brief Parallel For implementation for independent tasks.
     * Blocks until all items are processed.
     */
    template<typename T, typename F>
    void parallelFor(std::vector<T>& items, F&& func) {
        if (items.empty()) return;
        if (items.size() == 1) {
            func(items[0]);
            return;
        }

        std::atomic<size_t> completedCount{0};
        size_t totalCount = items.size();
        
        for (auto& item : items) {
            enqueue([&item, &func, &completedCount]() {
                func(item);
                completedCount++;
            });
        }

        // Spin-wait (or wait on condition) for completion
        // For real-time audio, we want a very efficient sync.
        while (completedCount < totalCount) {
            std::this_thread::yield();
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_stop = true;
        }
        m_condition.notify_all();
        for (std::thread& worker : m_workers) worker.join();
    }

private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_stop;
    std::atomic<size_t> m_completedTasks{0};
};

} // namespace Beam

#endif
