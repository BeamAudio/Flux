#ifndef TUBE_COMPRESSOR_UI_HPP
#define TUBE_COMPRESSOR_UI_HPP

#include "audio_module.hpp"
#include "../session/asset_manager.hpp"

namespace Beam {

class TubeCompressorUI : public AudioModule {
public:
    TubeCompressorUI(std::shared_ptr<FluxNode> node, size_t id, float x, float y) 
        : AudioModule(node, id, x, y) {
        
        setBounds(x, y, 220, 280);
        
        // Create a brushed metal procedural texture if not exists
        initMetalTexture();
    }

    void render(QuadBatcher& batcher, float dt) override {
        // Faceplate (Brushed Metal)
        if (m_faceplate) {
            batcher.drawTexture(m_faceplate->getID(), m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 0, 0, 1, 1, 0.8f, 0.8f, 0.82f, 1.0f);
        } else {
            batcher.drawRoundedRect(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 5.0f, 1.0f, 0.3f, 0.3f, 0.32f, 1.0f);
        }

        // Screws in corners
        auto drawScrew = [&](float sx, float sy) {
            batcher.drawRoundedRect(sx, sy, 8, 8, 4.0f, 0.5f, 0.1f, 0.1f, 0.1f, 1.0f);
            batcher.drawLine(sx + 2, sy + 4, sx + 6, sy + 4, 1.0f, 0.4f, 0.4f, 0.4f, 1.0f);
        };
        drawScrew(m_bounds.x + 5, m_bounds.y + 5);
        drawScrew(m_bounds.x + m_bounds.w - 13, m_bounds.y + 5);
        drawScrew(m_bounds.x + 5, m_bounds.y + m_bounds.h - 13);
        drawScrew(m_bounds.x + m_bounds.w - 13, m_bounds.y + m_bounds.h - 13);

        // Name plate
        batcher.drawRoundedRect(m_bounds.x + 40, m_bounds.y + 15, m_bounds.w - 80, 25, 2.0f, 0.5f, 0.1f, 0.1f, 0.1f, 1.0f);
        batcher.drawText("VINTAGE TUBE", m_bounds.x + 65, m_bounds.y + 22, 10, 0.8f, 0.6f, 0.2f, 1.0f);

        // Internal rendering (knobs, ports)
        AudioModule::render(batcher, dt);
    }

private:
    void initMetalTexture() {
        // Simple noise-based brushed metal generator
        int w = 128, h = 128;
        std::vector<unsigned char> data(w * h * 3);
        for(int y=0; y<h; ++y) {
            for(int x=0; x<w; ++x) {
                float n = 0.8f + 0.2f * (float)rand()/RAND_MAX;
                // Add horizontal streaks
                if (rand() % 10 == 0) n *= 0.9f;
                
                int idx = (y * w + x) * 3;
                data[idx] = data[idx+1] = data[idx+2] = (unsigned char)(n * 255);
            }
        }
        m_faceplate = std::make_shared<Texture>("procedural_metal");
        m_faceplate->createRGB(w, h, data.data());
    }

    std::shared_ptr<Texture> m_faceplate;
};

} // namespace Beam

#endif // TUBE_COMPRESSOR_UI_HPP



