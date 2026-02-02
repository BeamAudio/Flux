#ifndef FLUX_TRACK_NODE_HPP
#define FLUX_TRACK_NODE_HPP

#include "engine/core/flux_node.hpp"
#include "engine/nodes/track_node.hpp"

namespace Beam {

class FluxTrackProcessor : public FluxProcessor {
public:
    FluxTrackProcessor(std::shared_ptr<TrackNode> track) : m_track(track) {}

    void process(const float** inputs, float** outputs, int frames) override {
        float* out = outputs[0];
        const float* in = inputs[0];

        if (m_track->getState() == TrackState::Playing) {
            // Playback mode: fill out with disk data (advances cursor and applies tape physics)
            m_track->process(out, frames, 2, (size_t)-1);
        } else if (m_track->getState() == TrackState::Recording && in) {
            // Recording mode: capture 'in' and pass to 'out' (applies tape physics)
            m_track->processRecording(in, out, frames, 2);
        } else if (in) {
            // Idle but with input: copy for monitoring and apply tape effects
            std::copy(in, in + frames * 2, out);
            m_track->process(out, frames, 2, (size_t)-1);
        } else {
            // Silent idle
            std::fill(out, out + frames * 2, 0.0f);
        }
    }

    void updateParameters(const float* params) override {
        // Map order: Tape Drive (0), Tape Age (1)
        m_track->setTapeParams(params[0], params[1]);
    }

    void prepare(float sampleRate, int maxBlockSize) override {
        // m_track doesn't need prepare currently
    }

    void setCurrentFrame(size_t frame) { m_currentFrame = frame; }

private:
    std::shared_ptr<TrackNode> m_track;
    size_t m_currentFrame = 0;
};

class FluxTrackNode : public FluxNode {
public:
    FluxTrackNode(const std::string& name, int bufferSize) : m_name(name) {
        m_track = std::make_shared<TrackNode>(name);
        
        addParameter(std::make_shared<Parameter>("Tape Drive", 0.0f, 2.0f, 0.0f));
        addParameter(std::make_shared<Parameter>("Tape Age", 0.0f, 1.0f, 0.0f));
    }

    std::unique_ptr<FluxProcessor> createProcessor() override {
        return std::make_unique<FluxTrackProcessor>(m_track);
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

    void onTransportStateChanged(bool playing) override {
        if (m_track->getState() != TrackState::Recording) {
            m_track->setState(playing ? TrackState::Playing : TrackState::Idle);
        }
    }

    void onTransportSeek(size_t frame) override {
        m_track->seek(frame);
    }

    void setState(TrackState state) { m_track->setState(state); }
    TrackState getState() const { return m_track->getState(); }

    std::vector<std::vector<float>> getPeakData(int numPoints) {
        return m_track->getPeakData(numPoints);
    }

    std::shared_ptr<TrackNode> getInternalNode() { return m_track; }

    std::string getName() const override { return m_name; }
    std::vector<Port> getInputPorts() const override { return { {"Stereo In", 2} }; }
    std::vector<Port> getOutputPorts() const override { return { {"Stereo Out", 2} }; }

private:
    std::string m_name;
    std::shared_ptr<TrackNode> m_track;
};




} // namespace Beam

#endif // FLUX_TRACK_NODE_HPP






