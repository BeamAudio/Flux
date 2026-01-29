#ifndef AUDIO_NODE_HPP
#define AUDIO_NODE_HPP

#include <vector>
#include <string>

namespace Beam {

class AudioNode {
public:
    virtual ~AudioNode() = default;
    
    virtual void process(float* buffer, int frames, int channels, size_t startFrame = 0) = 0;
    virtual std::string getName() const = 0;

    void setBypass(bool bypass) { m_isBypassed = bypass; }
    bool isBypassed() const { return m_isBypassed; }

protected:
    bool m_isBypassed = false;
};

} // namespace Beam

#endif // AUDIO_NODE_HPP



