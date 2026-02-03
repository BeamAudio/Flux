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
    RenderModal(const std::string& path, std::shared_ptr<RenderPlan> plan, size_t totalFrames) 
        : m_path(path) 
    {
        std::cout << "[RenderModal] Constructor entry" << std::endl; std::cout.flush();
        try {
            m_renderer = std::make_shared<OfflineRenderer>();
            std::cout << "[RenderModal] Renderer created" << std::endl; std::cout.flush();

            std::cout << "[RenderModal] Creating TextButton..." << std::endl; std::cout.flush();
            m_cancelBtn = std::make_shared<TextButton>("CANCEL");
            std::cout << "[RenderModal] TextButton created" << std::endl; std::cout.flush();

            std::cout << "[RenderModal] Calling setName..." << std::endl; std::cout.flush();
            setName("RenderModal");
            std::cout << "[RenderModal] Calling setBounds..." << std::endl; std::cout.flush();
            setBounds(0, 0, 400, 200);
            
            std::cout << "[RenderModal] Adding child component..." << std::endl; std::cout.flush();
            addChildComponent(m_cancelBtn);
            std::cout << "[RenderModal] Child component added" << std::endl; std::cout.flush();
            
            std::cout << "[RenderModal] Setting onClick callback..." << std::endl; std::cout.flush();
            m_cancelBtn->onClick([this]() {
                std::cout << "[RenderModal] Cancel clicked" << std::endl; std::cout.flush();
                m_renderer->cancel();
                m_isRendering = false;
                if (onClose) onClose();
            });

            // Start Rendering
            std::cout << "[RenderModal] Starting renderer..." << std::endl; std::cout.flush();
            if (!m_renderer->start(path, plan, totalFrames)) {
                std::cout << "[RenderModal] Renderer failed to start" << std::endl; std::cout.flush();
                m_status = "Error starting render!";
                m_progress = 0.0f;
            } else {
                std::cout << "[RenderModal] Renderer started successfully" << std::endl; std::cout.flush();
                m_status = "Rendering...";
                m_isRendering = true;
            }
        } catch (const std::exception& e) {
            std::cerr << "[RenderModal] ERROR in constructor: " << e.what() << std::endl; std::cerr.flush();
            throw;
        } catch (...) {
            std::cerr << "[RenderModal] UNKNOWN ERROR in constructor" << std::endl; std::cerr.flush();
            throw;
        }
    }
    
    ~RenderModal() {
        if(m_isRendering) m_renderer->cancel();
    }

    void update(float dt) override {
        if (m_isRendering) {
            bool done = m_renderer->processChunk(4096); 
            m_progress = m_renderer->getProgress();
            
            if (done) {
                m_isRendering = false;
                m_status = "Complete!";
                if (m_cancelBtn) m_cancelBtn->setButtonText("CLOSE");
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
        if (m_cancelBtn)
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
