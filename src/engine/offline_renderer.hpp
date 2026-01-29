#ifndef OFFLINE_RENDERER_HPP
#define OFFLINE_RENDERER_HPP

#include "flux_graph.hpp"
#include "../../third_party/dr_wav.h"
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
            for (auto& op : plan->clearOps) {
                std::fill(op.node->getInputBuffer(op.portIdx), op.node->getInputBuffer(op.portIdx) + blockFrames * 2, 0.0f);
            }

            // 2. Process Nodes
            for (auto& exec : plan->sequence) {
                exec.node->setCurrentFrame(currentFrame);
                exec.node->process(blockFrames);

                for (auto& route : exec.outgoingRoutes) {
                    float* src = route.sourceNode->getOutputBuffer(route.sourcePort);
                    float* dst = route.destNode->getInputBuffer(route.destPort);
                    for (int i = 0; i < blockFrames * 2; ++i) dst[i] += src[i];
                }
            }

            // 3. Extract Master Output (Assuming Master is the last node or find it)
            // For now, assume we find the MasterNode in the plan
            std::shared_ptr<MasterNode> master;
            for(auto& exec : plan->sequence) {
                if (auto m = std::dynamic_pointer_cast<MasterNode>(exec.node)) {
                    master = m; break;
                }
            }

            if (master) {
                float* masterOut = master->getInputBuffer(0); // Master sums into its input
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
