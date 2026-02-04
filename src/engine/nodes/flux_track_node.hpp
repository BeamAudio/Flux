#ifndef FLUX_TRACK_NODE_HPP
#define FLUX_TRACK_NODE_HPP

#include "engine/core/flux_node.hpp"
#include "engine/nodes/track_node.hpp"
#include "engine/session/region.hpp"

namespace Beam {

class FluxTrackProcessor : public FluxProcessor {
public:
    FluxTrackProcessor(std::shared_ptr<TrackNode> track) : m_track(track) {}

    void process(const float** inputs, float** outputs, int frames) override {
        float* out = outputs[0];
        const float* in = inputs[0];

        bool playedAny = false;

        if (m_track->getState() == TrackState::Playing) {
            // --- Region Based Playback ---
            // Find if current frame falls into any region
            for (const auto& reg : m_regions) {
                if (m_currentFrame >= reg.startFrame && m_currentFrame < reg.startFrame + reg.duration) {
                    // Calculate frame in source file
                    size_t offsetInRegion = m_currentFrame - reg.startFrame;
                    size_t fileFrame = reg.sourceOffset + offsetInRegion;
                    
                    m_track->process(out, frames, 2, fileFrame);
                    playedAny = true;
                    break;
                }
            }
            
            if (!playedAny) {
                std::fill(out, out + frames * 2, 0.0f);
            }
        } else if (m_track->getState() == TrackState::Recording && in) {
            m_track->processRecording(in, out, frames, 2);
            playedAny = true;
        } else if (in) {
            std::copy(in, in + frames * 2, out);
            m_track->process(out, frames, 2, (size_t)-1);
            playedAny = true;
        } else {
            std::fill(out, out + frames * 2, 0.0f);
        }

        // --- Handle Sends ---
        // outputs[1] = Send 1, outputs[2] = Send 2
        for (int s = 1; s <= 2; ++s) {
            if (outputs[s]) {
                float sendGain = m_sendLevels[s - 1];
                for (int i = 0; i < frames * 2; ++i) {
                    outputs[s][i] = out[i] * sendGain;
                }
            }
        }
    }

    void updateParameters(const float* params) override {
        // Map order: Tape Drive (0), Tape Age (1), Send 1 (2), Send 2 (3)
        m_track->setTapeParams(params[0], params[1]);
        m_sendLevels[0] = params[2];
        m_sendLevels[1] = params[3];
    }

    void prepare(float sampleRate, int maxBlockSize) override {
        // m_track doesn't need prepare currently
    }

    void setCurrentFrame(size_t frame) override { m_currentFrame = frame; }
    
    void setRegions(const std::vector<Region>& regions) {
        m_regions = regions;
    }

private:
    std::shared_ptr<TrackNode> m_track;
    size_t m_currentFrame = 0;
    float m_sendLevels[2] = {0.0f, 0.0f};
    std::vector<Region> m_regions;
};

class FluxTrackNode : public FluxNode {
public:
    FluxTrackNode(const std::string& name, int bufferSize) : m_name(name) {
        m_track = std::make_shared<TrackNode>(name);
        
        addParameter(std::make_shared<Parameter>("Tape Drive", 0.0f, 2.0f, 0.0f));
        addParameter(std::make_shared<Parameter>("Tape Age", 0.0f, 1.0f, 0.0f));
        addParameter(std::make_shared<Parameter>("Send 1 Level", 0.0f, 1.0f, 0.0f));
        addParameter(std::make_shared<Parameter>("Send 2 Level", 0.0f, 1.0f, 0.0f));
    }

    std::shared_ptr<FluxProcessor> createProcessor() override {
        auto proc = std::make_shared<FluxTrackProcessor>(m_track);
        proc->setRegions(m_regions);
        return proc;
    }

    void setRegions(const std::vector<Region>& regions) {
        m_regions = regions;
        // If we have an active processor, it will be recreated on next plan update.
        // But for now, we just store them.
    }

    bool load(const std::string& filePath) {
        m_audioFilePath = filePath;
        return m_track->load(filePath);
    }

    // Serialization Overrides
    nlohmann::json serialize() const override {
        nlohmann::json data = FluxNode::serialize();
        data["type"] = "FluxTrackNode";
        data["audioFilePath"] = m_audioFilePath;
        return data;
    }

    void deserialize(const nlohmann::json& data) override {
        FluxNode::deserialize(data);
        if (data.contains("audioFilePath")) {
            std::string path = data["audioFilePath"];
             // Only load if path is valid and different? 
             // For restoration, we always load.
             if (!path.empty()) {
                 load(path);
             }
        }
    }

    bool startRecording(const std::string& filePath, int sampleRate) {
        m_audioFilePath = filePath;
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

    void setName(const std::string& name) {
        m_name = name;
        if (m_track) m_track->setName(name);
    }

    std::string getName() const override { return m_name; }
    std::vector<Port> getInputPorts() const override { return { {"Stereo In", 2} }; }
    std::vector<Port> getOutputPorts() const override { 
        return { 
            {"Stereo Out", 2},
            {"Send 1", 2},
            {"Send 2", 2}
        }; 
    }

private:
    std::string m_name;
    std::string m_audioFilePath;
    std::shared_ptr<TrackNode> m_track;
    std::vector<Region> m_regions;
};




} // namespace Beam

#endif // FLUX_TRACK_NODE_HPP






