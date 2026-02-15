#ifndef OFFLINE_RENDERER_HPP
#define OFFLINE_RENDERER_HPP

#include "engine/core/render_plan.hpp"
#include "dr_wav.h"
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

namespace Beam {

/**
 * @class OfflineRenderer
 * @brief Renders a provided RenderPlan to a file as fast as possible.
 */
class OfflineRenderer {
public:
    OfflineRenderer() {
        std::cout << "[OfflineRenderer] Constructor" << std::endl;
        std::cout.flush();
        std::memset(&m_wav, 0, sizeof(m_wav));
        m_paramCache.resize(4096, 0.0f);
    }
    ~OfflineRenderer() { 
        std::cout << "[OfflineRenderer] Destructor" << std::endl;
        std::cout.flush();
        if(m_active) cancel(); 
    }

    bool start(const std::string& filePath, std::shared_ptr<RenderPlan> plan, size_t totalFrames, int sampleRate = 44100) {
        std::cout << "[OfflineRenderer] start() entry" << std::endl;
        std::cout << "[OfflineRenderer] Target file: " << filePath << std::endl;
        std::cout.flush();
        if (!plan) {
            std::cerr << "[OfflineRenderer] No active plan provided!" << std::endl;
            return false;
        }
        
        m_plan = plan;
        m_totalFrames = totalFrames;
        m_currentFrame = 0;
        
        drwav_data_format format;
        format.container = drwav_container_riff;
        format.format = DR_WAVE_FORMAT_PCM;
        format.channels = 2;
        format.sampleRate = (drwav_uint32)sampleRate;
        format.bitsPerSample = 16;

        if (!drwav_init_file_write(&m_wav, filePath.c_str(), &format, nullptr)) {
            std::cerr << "[OfflineRenderer] Failed to initialize wav writer." << std::endl;
            return false;
        }

        std::cout << "[OfflineRenderer] Notifying nodes of transport start..." << std::endl;
        std::cout.flush();
        for (auto& node : m_plan->allNodes) {
            if (node) {
                node->onTransportStateChanged(true);
                node->onTransportSeek(0);
            }
        }

        m_active = true;
        std::cout << "[OfflineRenderer] Initialization complete. Block Size: " << m_plan->blockSize << std::endl;
        std::cout.flush();
        return true;
    }

    // Returns true if rendering is complete
    bool processChunk(int maxBlocks = 4) {
        if (!m_active || !m_plan) return true;
        
        int blocksProcessed = 0;
        int framesPerBlock = m_plan->blockSize;
        if (framesPerBlock <= 0) framesPerBlock = 512; // Failsafe

        while(blocksProcessed < maxBlocks && m_currentFrame < m_totalFrames) {
            int blockFrames = (int)(std::min)((size_t)framesPerBlock, m_totalFrames - m_currentFrame);
            
            // 1. Clear inputs
            for (int idx : m_plan->clearBufferIndices) {
                 if (idx >= 0 && idx < (int)m_plan->bufferPool.size()) {
                    auto& buf = m_plan->bufferPool[idx];
                    std::fill(buf.begin(), buf.end(), 0.0f);
                 }
            }

            // 2. Prepare Parameter Ramps
            for (auto& level : m_plan->levels) {
                for (auto& exec : level) {
                    for (auto* p : exec.parameters) {
                        if (p) p->prepareRamp(blockFrames);
                    }
                }
            }

            // 3. Process Levels
            for (auto& level : m_plan->levels) {
                for (auto& exec : level) {
                    if (!exec.processor) continue;

                    // Parameter update (using atomic targets as fallback/sync)
                    size_t numParams = exec.parameterPointers.size();
                    for(size_t i=0; i<numParams && i<4096; ++i) {
                        if (exec.parameterPointers[i])
                            m_paramCache[i] = exec.parameterPointers[i]->load(std::memory_order_relaxed);
                        else
                            m_paramCache[i] = 0.0f;
                    }
                    exec.processor->updateParameters(m_paramCache.data());
                    
                    const float* inputs[64]; 
                    float* outputs[64];
                    
                    size_t inCount = exec.inputBufferIndices.size();
                    size_t outCount = exec.outputBufferIndices.size();

                    for (size_t i = 0; i < inCount && i < 64; ++i) {
                        int idx = exec.inputBufferIndices[i];
                        inputs[i] = (idx >= 0 && idx < (int)m_plan->bufferPool.size()) ? m_plan->bufferPool[idx].data() : nullptr;
                    }
                    
                    for (size_t i = 0; i < outCount && i < 64; ++i) {
                        int idx = exec.outputBufferIndices[i];
                        outputs[i] = (idx >= 0 && idx < (int)m_plan->bufferPool.size()) ? m_plan->bufferPool[idx].data() : nullptr;
                    }

                    exec.processor->process(inputs, outputs, blockFrames);

                    // Routing (Serial within the level loop for offline consistency)
                    for (auto& route : exec.outgoingRoutes) {
                        if (route.mutePtr && route.mutePtr->load(std::memory_order_relaxed)) continue;
                        float gain = route.gainPtr ? route.gainPtr->load(std::memory_order_relaxed) : 1.0f;
                        float pan = route.panPtr ? route.panPtr->load(std::memory_order_relaxed) : 0.5f;
                        
                        int srcIdx = route.sourceBufferIdx;
                        int dstIdx = route.destBufferIdx;

                        if (srcIdx < 0 || srcIdx >= (int)m_plan->bufferPool.size() ||
                            dstIdx < 0 || dstIdx >= (int)m_plan->bufferPool.size()) continue;

                        float* src = m_plan->bufferPool[srcIdx].data();
                        float* dst = m_plan->bufferPool[dstIdx].data();
                        
                        // Offline renderer currently uses simple mixing, but we should match pan logic
                        float p = pan * 1.570796f;
                        float panL = std::cos(p);
                        float panR = std::sin(p);

                        // Mono-to-Stereo or Stereo-to-Stereo addition
                        for (int s = 0; s < blockFrames; ++s) {
                            dst[s * 2] += src[s * 2] * gain * panL;
                            dst[s * 2 + 1] += src[s * 2 + 1] * gain * panR;
                        }
                    }
                }
            }

            // 3. Write Output
            if(!m_plan->masterOutputBufferIndices.empty()) {
                int masterIdx = m_plan->masterOutputBufferIndices[0];
                if (masterIdx >= 0 && masterIdx < (int)m_plan->bufferPool.size()) {
                    float* masterOut = m_plan->bufferPool[masterIdx].data(); 
                    std::vector<int16_t> pcm(blockFrames * 2);
                    for (int i = 0; i < blockFrames * 2; ++i) {
                        float s = masterOut[i];
                        s = (std::min)(1.0f, (std::max)(-1.0f, s));
                        pcm[i] = (int16_t)(s * 32767.0f);
                    }
                    drwav_write_pcm_frames(&m_wav, (drwav_uint64)blockFrames, pcm.data());
                }
            }

            m_currentFrame += blockFrames;
            blocksProcessed++;
        }

        if (m_currentFrame >= m_totalFrames) {
            finish();
            return true;
        }
        return false;
    }

    void finish() {
        if (m_active) {
            if (m_plan) {
                for (auto& node : m_plan->allNodes) {
                    if (node) node->onTransportStateChanged(false);
                }
            }
            drwav_uninit(&m_wav);
            m_active = false;
            std::cout << "[OfflineRenderer] Render complete." << std::endl;
            std::cout.flush();
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
    std::shared_ptr<RenderPlan> m_plan;
    size_t m_currentFrame = 0;
    size_t m_totalFrames = 0;
    std::vector<float> m_paramCache;
};

} // namespace Beam

#endif // OFFLINE_RENDERER_HPP