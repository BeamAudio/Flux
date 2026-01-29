#ifndef DYNAMICS_MODULE_HPP
#define DYNAMICS_MODULE_HPP

#include "audio_module.hpp"
#include "../engine/analog_suite.hpp"

namespace Beam {

/**
 * @class DynamicsModule
 * @brief Specialized UI for Compressors/Limiters with Gain Reduction metering.
 */
template<typename T>
class DynamicsModule : public AudioModule {
public:
    DynamicsModule(std::shared_ptr<T> node, size_t nodeId, float x, float y) 
        : AudioModule(node, nodeId, x, y), m_dynamicsNode(node) {
        setBounds(x, y, 150, 240);
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        AudioModule::render(batcher, dt, screenW, screenH);
        
        // GR Meter
        float gr = m_dynamicsNode->getLatestGR();
        float meterW = 100;
        float meterH = 15;
        float mx = m_bounds.x + 25;
        float my = m_bounds.y + 45;

        // Background
        batcher.drawRoundedRect(mx, my, meterW, meterH, 2.0f, 0.5f, 0.05f, 0.05f, 0.05f, 1.0f);
        
        // GR active area (from right to left)
        float grNorm = std::clamp(gr * 0.1f, 0.0f, 1.0f); // Map roughly to 0..1
        float activeW = meterW * grNorm;
        batcher.drawRoundedRect(mx + meterW - activeW, my, activeW, meterH, 2.0f, 0.5f, 1.0f, 0.2f, 0.2f, 1.0f);
        
        batcher.drawText("GR", mx - 15, my + 2, 9, 0.6f, 0.6f, 0.6f, 1.0f);
    }

private:
    std::shared_ptr<T> m_dynamicsNode;
};

} // namespace Beam

#endif // DYNAMICS_MODULE_HPP
