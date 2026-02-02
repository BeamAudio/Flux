#include "interface/modules/audio_module.hpp"
#include "interface/core/theme.hpp"

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
    
    // Header
    batcher.drawRoundedGradientRect(x + 5, y + 5, w - 10, 24, 4.0f, 0.5f, 
                                   Theme::Aluminum.r, Theme::Aluminum.g, Theme::Aluminum.b, 1.0f,
                                   Theme::Aluminum.darker(0.3f).r, Theme::Aluminum.darker(0.3f).g, Theme::Aluminum.darker(0.3f).b, 1.0f);

    // Title (Engraved-style dark text)
    batcher.drawText(getName(), x + 15, y + 11, 12, 0.1f, 0.1f, 0.15f, 0.9f);

    // Delete Button (X)
    if (getName() != "Master" && getName() != "MASTER") {
        float delX = w - 25; // Aligned with deleteBtnBounds logic
        float delY = 0;
        // Draw matched to the hit box roughly
        batcher.drawRoundedRect(delX + 2, delY + 4, 20, 20, 3.0f, 0.5f, Theme::Red.darker(0.2f).r, Theme::Red.darker(0.2f).g, Theme::Red.darker(0.2f).b, 1.0f);
        batcher.drawText("X", delX + 8, delY + 7, 11, 1.0f, 1.0f, 1.0f, 0.9f);
    }
}

} // namespace Beam