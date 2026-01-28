#ifndef DSP_UTILS_HPP
#define DSP_UTILS_HPP

#include <cmath>
#include <algorithm>

namespace Beam {

/**
 * @brief Ensures a floating point value is not a "denormal" number.
 * 
 * Denormals (numbers very close to zero) can cause massive CPU performance 
 * hits on modern processors. This function flushes them to zero.
 */
inline float flush_denormal(float value) {
    const float threshold = 1e-15f;
    if (std::abs(value) < threshold) return 0.0f;
    return value;
}

/**
 * @brief Clips a value between a minimum and maximum.
 */
inline float clip(float val, float min, float max) {
    return (std::max)(min, (std::min)(max, val));
}

} // namespace Beam

#endif // DSP_UTILS_HPP

