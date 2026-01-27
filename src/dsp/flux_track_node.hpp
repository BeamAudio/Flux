#ifndef FLUX_TRACK_NODE_HPP
#define FLUX_TRACK_NODE_HPP

#include "flux_node.hpp"
#include "track_node.hpp"

namespace Beam {

class FluxTrackNode : public FluxNode {
public:
    FluxTrackNode(const std::string& name, int bufferSize) : m_name(name) {
        m_track = std::make_shared<TrackNode>(name);
        setupBuffers(0, 1, bufferSize, 2); // 0 Inputs, 1 Stereo Output
    }

    bool load(const std::string& filePath) {
        return m_track->load(filePath);
    }

    void process(int frames) override {
        float* out = getOutputBuffer(0);
        std::fill(out, out + frames * 2, 0.0f);
        m_track->process(out, frames, 2);
    }

    void seek(size_t frame) {
        m_track->seek(frame);
    }

    void setState(TrackState state) {
        m_track->setState(state);
    }

    TrackState getState() const {
        return m_track->getState();
    }

    std::shared_ptr<TrackNode> getInternalNode() { return m_track; }

    std::string getName() const override { return m_name; }
    
    std::vector<Port> getInputPorts() const override {
        return {};
    }
    
    std::vector<Port> getOutputPorts() const override {
        return { {"Stereo Out", 2} };
    }

private:
    std::string m_name;
    std::shared_ptr<TrackNode> m_track;
};

} // namespace Beam

#endif // FLUX_TRACK_NODE_HPP
