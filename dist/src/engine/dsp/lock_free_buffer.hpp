#ifndef LOCK_FREE_BUFFER_HPP
#define LOCK_FREE_BUFFER_HPP

#include <vector>
#include <atomic>
#include <algorithm>

namespace Beam {

/**
 * @class LockFreeBuffer
 * @brief Single-producer, single-consumer lock-free circular buffer for audio data.
 */
template <typename T>
class LockFreeBuffer {
public:
    LockFreeBuffer(size_t capacity) : m_buffer(capacity), m_capacity(capacity) {
        m_writePos.store(0);
        m_readPos.store(0);
    }

    /**
     * @brief Writes data to the buffer.
     * @return Number of elements actually written.
     */
    size_t write(const T* data, size_t count) {
        size_t write = m_writePos.load(std::memory_order_relaxed);
        size_t read = m_readPos.load(std::memory_order_acquire);

        // Calculate available space
        size_t available;
        if (write >= read) available = m_capacity - (write - read) - 1;
        else available = read - write - 1;

        size_t toWrite = (std::min)(count, available);
        if (toWrite == 0) return 0;

        // Write in one or two segments
        size_t part1 = (std::min)(toWrite, m_capacity - write);
        std::copy(data, data + part1, m_buffer.begin() + write);
        
        if (part1 < toWrite) {
            std::copy(data + part1, data + toWrite, m_buffer.begin());
        }

        m_writePos.store((write + toWrite) % m_capacity, std::memory_order_release);
        return toWrite;
    }

    /**
     * @brief Reads data from the buffer.
     * @return Number of elements actually read.
     */
    size_t read(T* data, size_t count) {
        size_t write = m_writePos.load(std::memory_order_acquire);
        size_t read = m_readPos.load(std::memory_order_relaxed);

        size_t available = (write >= read) ? (write - read) : (m_capacity - read + write);
        size_t toRead = (std::min)(count, available);
        if (toRead == 0) return 0;

        size_t part1 = (std::min)(toRead, m_capacity - read);
        std::copy(m_buffer.begin() + read, m_buffer.begin() + read + part1, data);

        if (part1 < toRead) {
            std::copy(m_buffer.begin(), m_buffer.begin() + (toRead - part1), data + part1);
        }

        m_readPos.store((read + toRead) % m_capacity, std::memory_order_release);
        return toRead;
    }

    size_t getAvailableRead() const {
        size_t write = m_writePos.load(std::memory_order_acquire);
        size_t read = m_readPos.load(std::memory_order_relaxed);
        return (write >= read) ? (write - read) : (m_capacity - read + write);
    }

    size_t getAvailableWrite() const {
        size_t write = m_writePos.load(std::memory_order_relaxed);
        size_t read = m_readPos.load(std::memory_order_acquire);
        size_t available;
        if (write >= read) available = m_capacity - (write - read) - 1;
        else available = read - write - 1;
        return available;
    }

    void reset() {
        // Not strictly thread-safe if producer/consumer contest, but useful for initialization/reset
        m_readPos.store(m_writePos.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }

private:
    std::vector<T> m_buffer;
    size_t m_capacity;
    std::atomic<size_t> m_writePos;
    std::atomic<size_t> m_readPos;
};

} // namespace Beam

#endif // LOCK_FREE_BUFFER_HPP
