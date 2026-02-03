#ifndef AUDIO_ENGINE_HPP
#define AUDIO_ENGINE_HPP

#include "engine/core/audio_node.hpp"
#include "engine/core/flux_graph.hpp"
#include "engine/core/render_plan.hpp"
#include "engine/nodes/master_node.hpp"
#include "engine/nodes/input_node.hpp"
#include "engine/session/automation.hpp"
#include "engine/session/mixer_state.hpp"
#include <SDL3/SDL.h>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>

namespace Beam {

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    int getBlockSize() const { return m_blockSize; }
    int getChannels() const { return m_channels; }
    
    bool init(int sampleRate, int channels, int blockSize, const std::string& outputDevice = "", const std::string& inputDevice = "");
    
    void audioCallback(const std::map<std::string, float*>& inputs, float** outputs, int frames, int inChannels, int outChannels, double sampleRate);

    void setGraph(std::shared_ptr<FluxGraph> graph);
    void setMasterNodeId(size_t id);
    void updatePlan();
    
    std::shared_ptr<FluxGraph> getGraph() { return m_graph; }
    std::shared_ptr<MasterNode> getMasterNode() { 
        if (m_graph) {
            auto node = m_graph->getNode(m_masterNodeId);
            if (auto master = std::dynamic_pointer_cast<MasterNode>(node)) {
                return master;
            }
        }
        return m_masterNode; 
    }
    size_t getMasterNodeId() const { return m_masterNodeId; }

    void setPlaying(bool playing);
    bool isPlaying() const { return m_isPlaying.load(); }
    void rewind();
    void seek(size_t frame);

    size_t getCurrentFrame() const { return m_currentFrame.load(); }

    void addAutomationLane(std::shared_ptr<AutomationLane> lane) {
        std::lock_guard<std::mutex> lock(m_automationMutex);
        m_automationLanes.push_back(lane);
    }

    std::shared_ptr<InputNode> getInputNode() { return m_inputNode; }
    
    MixerState& getMixerState() { return m_mixerState; }

private:
    std::atomic<size_t> m_currentFrame{0};
    std::atomic<int64_t> m_pendingSeek{-1};
    std::vector<std::shared_ptr<AutomationLane>> m_automationLanes;
    std::mutex m_automationMutex;
    
    int m_sampleRate;
    int m_channels;
    int m_blockSize;
    
    std::shared_ptr<FluxGraph> m_graph; 
    size_t m_masterNodeId;
    std::shared_ptr<MasterNode> m_masterNode;
    std::shared_ptr<InputNode> m_inputNode;

    std::atomic<std::shared_ptr<RenderPlan>> m_activePlan;
    
    std::atomic<bool> m_isPlaying{false};
    std::atomic<bool> m_isMuted{false};
    bool m_lastPlaying{false};
    
    MixerState m_mixerState;

    float* process(int frames);
    std::vector<float> m_scratchBuffer;
};

} // namespace Beam

#endif // AUDIO_ENGINE_HPP