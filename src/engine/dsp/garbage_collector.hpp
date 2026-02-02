#ifndef GARBAGE_COLLECTOR_HPP
#define GARBAGE_COLLECTOR_HPP

#include <vector>
#include <memory>
#include <mutex>

namespace Beam {

/**
 * @class GarbageCollector
 * @brief Thread-safe utility to defer the destruction of objects (like RenderPlans).
 * 
 * Objects pushed here from the Audio Thread will be kept alive until the UI thread
 * calls 'collect()', ensuring no destruction (and thus no free()) happens in the 
 * high-priority context.
 */
class GarbageCollector {
public:
    static GarbageCollector& get() {
        static GarbageCollector instance;
        return instance;
    }

    template<typename T>
    void defer(std::shared_ptr<T> obj) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_trash.push_back(std::static_pointer_cast<void>(obj));
    }

    void collect() {
        std::vector<std::shared_ptr<void>> toDelete;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            toDelete.swap(m_trash);
        }
        // toDelete goes out of scope here and destroys the objects on the UI thread
    }

private:
    GarbageCollector() = default;
    std::vector<std::shared_ptr<void>> m_trash;
    std::mutex m_mutex;
};

} // namespace Beam

#endif // GARBAGE_COLLECTOR_HPP
