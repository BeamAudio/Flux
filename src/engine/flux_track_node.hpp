#ifndef FLUX_TRACK_NODE_HPP
#define FLUX_TRACK_NODE_HPP

#include "flux_node.hpp"
#include "track_node.hpp"

namespace Beam {

class FluxTrackNode : public FluxNode {
public:
    FluxTrackNode(const std::string& name, int bufferSize) : m_name(name) {
        m_track = std::make_shared<TrackNode>(name);
        setupBuffers(1, 1, bufferSize, 2); // 1 Stereo Input, 1 Stereo Output
    }

    bool load(const std::string& filePath) {
        return m_track->load(filePath);
    }

    bool startRecording(const std::string& filePath, int sampleRate) {
        return m_track->startRecording(filePath, sampleRate, 2);
    }

    void stopRecording() {
        m_track->stopRecording();
    }

    void process(int frames) override {
        float* in = getInputBuffer(0);
        float* out = getOutputBuffer(0);

        if (m_track->getState() == TrackState::Recording) {
            // Process recording: write 'in' to disk, and pass through to 'out' for monitoring
            m_track->process(in, frames, 2, m_currentFrame);
            std::copy(in, in + frames * 2, out);
        } else {
            // Process playback: read from disk into 'out'
            std::fill(out, out + frames * 2, 0.0f);
            m_track->process(out, frames, 2, m_currentFrame);
        }
    }

    void onTransportStateChanged(bool playing) override {
        if (m_track->getState() != TrackState::Recording) {
            m_track->setState(playing ? TrackState::Playing : TrackState::Idle);
        }
    }

    void onTransportSeek(size_t frame) override {
        seek(frame);
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

    std::vector<std::vector<float>> getPeakData(int numPoints) {
        return m_track->getPeakData(numPoints);
    }

    std::shared_ptr<TrackNode> getInternalNode() { return m_track; }

    std::string getName() const override { return m_name; }

    std::vector<Port> getInputPorts() const override {
        return { {"Stereo In", 2} };
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



