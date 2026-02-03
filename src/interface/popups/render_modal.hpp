#ifndef RENDER_MODAL_HPP
#define RENDER_MODAL_HPP

#include "interface/core/component.hpp"
#include "interface/core/theme.hpp"
#include "interface/widgets/button.hpp"
#include "engine/core/offline_renderer.hpp"
#include "interface/render/quad_batcher.hpp"
#include <string>
#include <functional>

namespace Beam {

class RenderModal : public Component {
public:
    RenderModal(const std::string& path, std::shared_ptr<FluxGraph> graph, size_t totalFrames, size_t masterNodeId = (size_t)-1) 
        : m_path(path), m_renderer(std::make_shared<OfflineRenderer>()) 
    {
        setName("RenderModal");
        setBounds(0, 0, 400, 200);

        m_cancelBtn = std::make_shared<TextButton>("CANCEL");
        addChildComponent(m_cancelBtn);
        
        m_cancelBtn->onClick([this]() {
            m_renderer->cancel();
            if (onClose) onClose();
        });

        // Start Rendering
        if (!m_renderer->start(path, graph, totalFrames, masterNodeId)) {
            m_status = "Error starting render!";
            m_progress = 0.0f;
        } else {
            m_status = "Rendering...";
            m_isRendering = true;
        }
    }
    
    ~RenderModal() {
        if(m_isRendering) m_renderer->cancel();
    }

    void update(float dt) override {
        if (m_isRendering) {
            bool done = m_renderer->processChunk(4096); // Reduced for responsiveness
            m_progress = m_renderer->getProgress();
            
            if (done) {
                m_isRendering = false;
                m_status = "Complete!";
                if (onClose) onClose(); // Auto-close? Or wait?
            }
        }
    }

    void paint(QuadBatcher& batcher) override {
        // Dim Background (Modal Overlay - handled by parent usually, but we are just a window here)
        // Draw Window
        batcher.drawRoundedRect(0.0f, 0.0f, m_bounds.w, m_bounds.h, 4.0f, 0.5f, 0.15f, 0.15f, 0.18f, 1.0f);
        batcher.drawQuad(0.0f, 0.0f, m_bounds.w, 30.0f, 0.1f, 0.1f, 0.12f, 1.0f); // Title bar
        batcher.drawText("Rendering Project...", 10.0f, 8.0f, 14.0f, 0.9f, 0.9f, 0.9f, 1.0f);

        // Status
        batcher.drawText(m_status, 20, 50, 16.0f, 0.8f, 0.8f, 0.8f, 1.0f);
        // Path
        batcher.drawText(m_path, 20, 75, 12.0f, 0.5f, 0.5f, 0.5f, 1.0f);

        // Progress Bar
        float barX = 20;
        float barY = 110;
        float barW = m_bounds.w - 40;
        float barH = 20;

        batcher.drawQuad(barX, barY, barW, barH, 0.1f, 0.1f, 0.1f, 1.0f); // Track
        batcher.drawQuad(barX, barY, barW * m_progress, barH, 0.9f, 0.5f, 0.2f, 1.0f); // Fill
        
        std::string pct = std::to_string((int)(m_progress * 100)) + "%";
        batcher.drawText(pct, barX + barW/2 - 10, barY + 2, 12.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    void resized() override {
        m_cancelBtn->setBounds(m_bounds.w - 100, m_bounds.h - 40, 80, 24);
    }

    std::function<void()> onClose;

private:
    std::string m_path;
    std::string m_status;
    float m_progress = 0.0f;
    bool m_isRendering = false;
    std::shared_ptr<OfflineRenderer> m_renderer;
    std::shared_ptr<TextButton> m_cancelBtn;
};

} // namespace Beam

#endif
