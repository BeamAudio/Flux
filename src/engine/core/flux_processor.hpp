#ifndef FLUX_PROCESSOR_HPP
#define FLUX_PROCESSOR_HPP

#include <vector>
#include <string>
#include "engine/midi/midi_event.hpp"

namespace Beam {

/**
 * @class FluxProcessor
 * @brief The real-time worker responsible for audio processing.
 * 
 * FluxProcessor is the "hot" half of a DSP entity. It lives strictly on the
 * audio thread and must follow absolute real-time constraints:
 * - NO allocations (malloc, new)
 * - NO locks (mutexes)
 * - NO blocking calls
 * - NO virtual calls in inner loops (except the main process() entry)
 */
class FluxProcessor {
public:
    virtual ~FluxProcessor() = default;

    /**
     * @brief Safe deallocation across DLL boundaries.
     */
    virtual void releaseNode() { delete this; }

    /**
     * @brief Main processing loop.
     * @param inputs Pointer to array of input buffers (L/R interleaved)
     * @param outputs Pointer to array of output buffers (L/R interleaved)
     * @param frames Number of frames to process
     */
    virtual void process(const float** inputs, float** outputs, int frames) = 0;

    /**
     * @brief MIDI processing loop.
     * @param midi Buffer containing MIDI events for the current block.
     */
    virtual void processMIDI(MIDIBuffer& midi) {}

    /**
     * @brief Handle transport state changes (Start, Stop, Seek).
     */
    virtual void reset() {}

    /**
     * @brief Update local parameters from the thread-safe handshake.
     * This is called by the engine just before process().
     */
    virtual void updateParameters(const float* paramValues) {}

    /**
     * @brief Sets the bypass state of this processor.
     */
    void setBypassed(bool bypassed) { m_bypassed.store(bypassed, std::memory_order_relaxed); }
    bool isBypassed() const { return m_bypassed.load(std::memory_order_relaxed); }

    /**
     * @brief Optimization: Prepare internal state for a specific sample rate/block size.
     */
    virtual void prepare(float sampleRate, int maxBlockSize) {}

    /**
     * @brief Informs the processor of the current global timeline frame.
     */
    virtual void setCurrentFrame(size_t frame) {}

protected:
    std::atomic<bool> m_bypassed{false};
};

} // namespace Beam

#endif // FLUX_PROCESSOR_HPP
