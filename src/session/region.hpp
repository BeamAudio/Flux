#ifndef REGION_HPP
#define REGION_HPP

#include <string>
#include <vector>

namespace Beam {

/**
 * @struct Region
 * @brief Represents a clip of audio on the timeline.
 */
struct Region {
    std::string name;
    size_t startFrame;  ///< Position on the timeline
    size_t duration;    ///< Length in frames
    size_t sourceOffset; ///< Offset into the source audio file
    int trackIndex;     ///< Vertical lane index
    std::vector<std::vector<float>> channelPeaks; ///< Cached waveform data per channel
};

} // namespace Beam

#endif // REGION_HPP



