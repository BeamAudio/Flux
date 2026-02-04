#include "interface/modules/audio_module.hpp"
#include "interface/core/theme.hpp"
#include "interface/views/workspace.hpp"

namespace Beam {

void AudioModule::paint(QuadBatcher& batcher) {
    // 1. Module Background (Local Coordinates 0,0)
    float x = 0.0f;
    float y = 0.0f;
    float w = m_bounds.w;
    float h = m_bounds.h;

    // Shadow
    batcher.drawRoundedRect(x + 4, y + 4, w, h, 6.0f, 12.0f, 0.0f, 0.0f, 0.0f, 0.4f);

    // Main Body (Console Grey) + Hardware Noise
    batcher.drawChassisPanel(x, y, w, h, 6.0f, Theme::Console.r, Theme::Console.g, Theme::Console.b, 1.0f);
    
    // Selection Highlight
    if (auto ws = dynamic_cast<class Workspace*>(getParent())) {
        if (ws->isNodeSelected(m_nodeId)) {
            batcher.drawRect(x - 2, y - 2, w + 4, h + 4, 2.0f, Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b, 0.8f);
        }
    }

    // Header
    batcher.drawRoundedGradientRect(x + 5, y + 5, w - 10, 24, 4.0f, 0.5f, 
                                   Theme::Aluminum.r, Theme::Aluminum.g, Theme::Aluminum.b, 1.0f,
                                   Theme::Aluminum.darker(0.3f).r, Theme::Aluminum.darker(0.3f).g, Theme::Aluminum.darker(0.3f).b, 1.0f);

    // Title (Engraved-style dark text)
    batcher.drawText(getName(), x + 15, y + 11, 12, 0.1f, 0.1f, 0.15f, 0.9f);

    // Header Controls (Delete and Bypass)
    if (getName() != "Master" && getName() != "MASTER") {
        // Delete Button (X)
        float delX = w - 25; 
        batcher.drawRoundedRect(delX + 2, 4, 20, 20, 3.0f, 0.5f, Theme::Red.darker(0.2f).r, Theme::Red.darker(0.2f).g, Theme::Red.darker(0.2f).b, 1.0f);
        batcher.drawText("X", delX + 8, 7, 11, 1.0f, 1.0f, 1.0f, 0.9f);

        // Bypass Button (B)
        float byX = w - 50;
        bool bypassed = m_node ? m_node->isBypassed() : false;
        Color byCol = bypassed ? Theme::GreyLight : Theme::Emerald;
        batcher.drawRoundedRect(byX + 2, 4, 20, 20, 3.0f, 0.5f, byCol.r, byCol.g, byCol.b, 1.0f);
        batcher.drawText("B", byX + 8, 7, 11, bypassed ? 0.5f : 1.0f, bypassed ? 0.5f : 1.0f, bypassed ? 0.5f : 1.0f, 0.9f);
    }
}

} // namespace Beam