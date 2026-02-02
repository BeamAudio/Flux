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
    static bool renderToWav(const std::string& filePath, std::shared_ptr<FluxGraph> graph, size_t totalFrames, int sampleRate = 44100) {
        if (!graph) return false;

        drwav_data_format format;
        format.container = drwav_container_riff;
        format.format = DR_WAVE_FORMAT_PCM;
        format.channels = 2;
        format.sampleRate = (drwav_uint32)sampleRate;
        format.bitsPerSample = 16;

        drwav wav;
        if (!drwav_init_file_write(&wav, filePath.c_str(), &format, nullptr)) {
            return false;
        }

        auto plan = graph->compile(1024, 2);
        std::vector<float> renderBuf(1024 * 2);
        size_t framesRemaining = totalFrames;
        size_t currentFrame = 0;

        std::cout << "Starting Offline Render: " << totalFrames << " frames..." << std::endl;

        while (framesRemaining > 0) {
            int blockFrames = (int)(std::min)((size_t)1024, framesRemaining);
            
            // 1. Clear inputs
            for (int idx : plan->clearBufferIndices) {
                 std::fill(plan->bufferPool[idx].begin(), plan->bufferPool[idx].end(), 0.0f);
            }

            // 2. Process Nodes
            float paramCache[64];
            for (auto& exec : plan->sequence) {
                 // Params
                 for(size_t i=0; i<exec.parameterPointers.size() && i<64; ++i) {
                     paramCache[i] = exec.parameterPointers[i]->load();
                 }
                 exec.processor->updateParameters(paramCache);
                 
                 // Buffers
                 const float* inputs[8]; float* outputs[8];
                 for(size_t i=0; i<exec.inputBufferIndices.size() && i<8; ++i) 
                     inputs[i] = plan->bufferPool[exec.inputBufferIndices[i]].data();
                 for(size_t i=0; i<exec.outputBufferIndices.size() && i<8; ++i) 
                     outputs[i] = plan->bufferPool[exec.outputBufferIndices[i]].data();

                 // Process
                 exec.processor->process(inputs, outputs, blockFrames);

                 // Signal Routing (Summing)
                 for (auto& route : exec.outgoingRoutes) {
                     float* src = plan->bufferPool[route.sourceBufferIdx].data();
                     float* dst = plan->bufferPool[route.destBufferIdx].data();
                     for (int i = 0; i < blockFrames * 2; ++i) dst[i] += src[i];
                 }
            }

            // 3. Extract Master Output
            if(!plan->masterOutputBufferIndices.empty()) {
                float* masterOut = plan->bufferPool[plan->masterOutputBufferIndices[0]].data(); 
                std::vector<int16_t> pcm(blockFrames * 2);
                for (int i = 0; i < blockFrames * 2; ++i) {
                    float s = masterOut[i];
                    if (s > 1.0f) s = 1.0f;
                    if (s < -1.0f) s = -1.0f;
                    pcm[i] = (int16_t)(s * 32767.0f);
                }
                drwav_write_pcm_frames(&wav, (drwav_uint64)blockFrames, pcm.data());
            }

            framesRemaining -= blockFrames;
            currentFrame += blockFrames;
        }

        drwav_uninit(&wav);
        std::cout << "Offline Render Complete: " << filePath << std::endl;
        return true;
    }
};

} // namespace Beam

#endif // OFFLINE_RENDERER_HPP
