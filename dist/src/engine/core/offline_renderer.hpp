#ifndef OFFLINE_RENDERER_HPP
#define OFFLINE_RENDERER_HPP

#include "engine/core/flux_graph.hpp"
#include "dr_wav.h"
#include <string>
#include <vector>
#include <iostream>

namespace Beam {

/**
 * @class OfflineRenderer
 * @brief Renders the DSP graph to a file as fast as possible.
 */
class OfflineRenderer {
public:
    OfflineRenderer() = default;
    ~OfflineRenderer() { if(m_active) cancel(); }

    bool start(const std::string& filePath, std::shared_ptr<FluxGraph> graph, size_t totalFrames, size_t masterNodeId = (size_t)-1, int sampleRate = 44100) {
        if (!graph) return false;
        m_graph = graph;
        m_totalFrames = totalFrames;
        m_currentFrame = 0;
        
        drwav_data_format format;
        format.container = drwav_container_riff;
        format.format = DR_WAVE_FORMAT_PCM;
        format.channels = 2;
        format.sampleRate = (drwav_uint32)sampleRate;
        format.bitsPerSample = 16;

        if (!drwav_init_file_write(&m_wav, filePath.c_str(), &format, nullptr)) {
            return false;
        }

        m_plan = m_graph->compile(1024, 2, masterNodeId);
        m_active = true;
        return true;
    }

    // Returns true if rendering is complete
    bool processChunk(int maxFrames = 4096) {
        if (!m_active) return true;
        
        int processedThisCall = 0;
        while(processedThisCall < maxFrames && m_currentFrame < m_totalFrames) {
            int blockFrames = (int)(std::min)((size_t)1024, m_totalFrames - m_currentFrame);
            
            // 1. Clear inputs
            for (int idx : m_plan->clearBufferIndices) {
                 std::fill(m_plan->bufferPool[idx].begin(), m_plan->bufferPool[idx].end(), 0.0f);
            }

            // 2. Process Nodes
            float paramCache[64];
            for (auto& exec : m_plan->sequence) {
                 for(size_t i=0; i<exec.parameterPointers.size() && i<64; ++i) 
                     paramCache[i] = exec.parameterPointers[i]->load(std::memory_order_relaxed);
                 exec.processor->updateParameters(paramCache);
                 
                 const float* inputs[8]; 
                 float* outputs[8];
                 
                 for (int i = 0; i < 8; ++i) {
                     if (i < (int)exec.inputBufferIndices.size()) {
                         inputs[i] = m_plan->bufferPool[exec.inputBufferIndices[i]].data();
                     } else {
                         inputs[i] = nullptr;
                     }
                 }
                 
                 for (int i = 0; i < 8; ++i) {
                     if (i < (int)exec.outputBufferIndices.size()) {
                         outputs[i] = m_plan->bufferPool[exec.outputBufferIndices[i]].data();
                     } else {
                         outputs[i] = nullptr;
                     }
                 }

                 exec.processor->process(inputs, outputs, blockFrames);

                 for (auto& route : exec.outgoingRoutes) {
                     // Check mute
                     if (route.mutePtr && route.mutePtr->load(std::memory_order_relaxed)) continue;

                     float gain = route.gainPtr ? route.gainPtr->load(std::memory_order_relaxed) : 1.0f;
                     float* src = m_plan->bufferPool[route.sourceBufferIdx].data();
                     float* dst = m_plan->bufferPool[route.destBufferIdx].data();
                     
                     if (gain == 1.0f) {
                         for (int i = 0; i < blockFrames * 2; ++i) dst[i] += src[i];
                     } else if (gain > 0.0f) {
                         for (int i = 0; i < blockFrames * 2; ++i) dst[i] += src[i] * gain;
                     }
                 }
            }

            // 3. Write Output
            if(!m_plan->masterOutputBufferIndices.empty()) {
                float* masterOut = m_plan->bufferPool[m_plan->masterOutputBufferIndices[0]].data(); 
                std::vector<int16_t> pcm(blockFrames * 2);
                for (int i = 0; i < blockFrames * 2; ++i) {
                    float s = masterOut[i];
                    s = (std::min)(1.0f, (std::max)(-1.0f, s));
                    pcm[i] = (int16_t)(s * 32767.0f);
                }
                drwav_write_pcm_frames(&m_wav, (drwav_uint64)blockFrames, pcm.data());
            }

            m_currentFrame += blockFrames;
            processedThisCall += blockFrames;
        }

        if (m_currentFrame >= m_totalFrames) {
            finish();
            return true;
        }
        return false;
    }

    void finish() {
        if (m_active) {
            drwav_uninit(&m_wav);
            m_active = false;
        }
    }

    void cancel() {
        finish();
    }

    float getProgress() const {
        if (m_totalFrames == 0) return 0.0f;
        return (float)m_currentFrame / (float)m_totalFrames;
    }

private:
    drwav m_wav;
    bool m_active = false;
    std::shared_ptr<FluxGraph> m_graph;
    std::shared_ptr<RenderPlan> m_plan;
    size_t m_currentFrame = 0;
    size_t m_totalFrames = 0;
};

} // namespace Beam

#endif // OFFLINE_RENDERER_HPP
