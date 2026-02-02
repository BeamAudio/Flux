#ifndef SIMD_UTILS_HPP
#define SIMD_UTILS_HPP

#include <immintrin.h>

namespace Beam {
namespace SIMD {

/**
 * @brief Adds two buffers using SSE (4 floats at a time).
 * @param src Source buffer.
 * @param dst Destination buffer (result is summed into this).
 * @param count Total number of samples (must be multiple of 4 for full optimization).
 */
inline void add(const float* src, float* dst, int count) {
    int i = 0;
    // Process 4 floats at a time
    for (; i <= count - 4; i += 4) {
        __m128 vsrc = _mm_loadu_ps(&src[i]);
        __m128 vdst = _mm_loadu_ps(&dst[i]);
        vdst = _mm_add_ps(vdst, vsrc);
        _mm_storeu_ps(&dst[i], vdst);
    }
    // Handle remaining samples
    for (; i < count; ++i) {
        dst[i] += src[i];
    }
}

/**
 * @brief Copies a buffer using SSE.
 */
inline void copy(const float* src, float* dst, int count) {
    int i = 0;
    for (; i <= count - 4; i += 4) {
        _mm_storeu_ps(&dst[i], _mm_loadu_ps(&src[i]));
    }
    for (; i < count; ++i) {
        dst[i] = src[i];
    }
}

} // namespace SIMD
} // namespace Beam

#endif // SIMD_UTILS_HPP






