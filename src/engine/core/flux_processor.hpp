#ifndef FLUX_PROCESSOR_HPP
#define FLUX_PROCESSOR_HPP

#include <vector>
#include <string>

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
     * @brief Main processing loop.
     * @param inputs Pointer to array of input buffers (L/R interleaved)
     * @param outputs Pointer to array of output buffers (L/R interleaved)
     * @param frames Number of frames to process
     */
    virtual void process(const float** inputs, float** outputs, int frames) = 0;

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
     * @brief Optimization: Prepare internal state for a specific sample rate/block size.
     */
    virtual void prepare(float sampleRate, int maxBlockSize) {}
};

} // namespace Beam

#endif // FLUX_PROCESSOR_HPP
