/**
 * @file beam_widgets.hpp
 * @brief SDK Widget Library for Beam Audio Flux plugins
 * 
 * Pre-built UI widgets for rapid plugin development:
 * - Knobs, Sliders, Buttons
 * - LEDs, VU Meters, Waveforms
 * - XY Pads, Graphs, Labels
 */

#ifndef BEAM_WIDGETS_HPP
#define BEAM_WIDGETS_HPP

// Prevent Windows min/max macros from conflicting with std::min/max
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include "interface/render/quad_batcher.hpp"
#include "engine/session/parameter.hpp"
#include "interface/core/theme.hpp"
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>

namespace Beam {
namespace SDK {

// LED color presets
enum LEDColor { LED_RED, LED_GREEN, LED_AMBER, LED_BLUE, LED_WHITE };

// Button style presets
enum ButtonStyle { BTN_MOMENTARY, BTN_TOGGLE, BTN_RADIO };

// =============================================================================
// DRAWING HELPERS
// =============================================================================

inline void getColorFromLED(LEDColor color, float& r, float& g, float& b) {
    switch (color) {
        case LED_RED:    r = 1.0f; g = 0.2f; b = 0.2f; break;
        case LED_GREEN:  r = 0.2f; g = 1.0f; b = 0.3f; break;
        case LED_AMBER:  r = 1.0f; g = 0.7f; b = 0.1f; break;
        case LED_BLUE:   r = 0.2f; g = 0.5f; b = 1.0f; break;
        case LED_WHITE:  r = 1.0f; g = 1.0f; b = 1.0f; break;
    }
}

// =============================================================================
// ADVANCED STYLING
// =============================================================================

struct PanelStyle {
    Color chassisColor;
    Color textColor;
    Theme::MaterialType material = Theme::MaterialType::Standard;
    bool showScrews = true;
    std::string title;
    std::string subtitle;
};

inline void drawPanel(QuadBatcher& g, float x, float y, float w, float h, const PanelStyle& s) {
    // Drop Shadow
    g.drawRoundedRect(x + 4, y + 5, w, h, 6.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.3f);
    
    // Main Chassis with Material Texture (SDF Mode 5)
    g.drawChassisPanel(x, y, w, h, 6.0f, s.chassisColor.r, s.chassisColor.g, s.chassisColor.b, 1.0f);
    
    // Screws
    if (s.showScrews) {
        auto drawScrew = [&](float sx, float sy) {
            g.drawRoundedRect(sx - 4 + 1, sy - 4 + 1, 8, 8, 4.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.4f);
            g.drawRoundedRect(sx - 4, sy - 4, 8, 8, 4.0f, 0.5f, 0.45f, 0.47f, 0.50f, 1.0f);
            g.drawLine(sx - 2, sy, sx + 2, sy, 1.5f, 0.1f, 0.1f, 0.1f, 0.8f);
        };
        drawScrew(x + 10, y + 10); drawScrew(x + w - 10, y + 10); 
        drawScrew(x + 10, y + h - 10); drawScrew(x + w - 10, y + h - 10);
    }

    // Titles
    if (!s.title.empty()) {
        g.drawText(s.title, x + 24, y + 10, 15.0f, s.textColor.darker(0.2f).r, s.textColor.darker(0.2f).g, s.textColor.darker(0.2f).b, 0.4f);
        g.drawText(s.title, x + 23, y + 9, 15.0f, s.textColor.r, s.textColor.g, s.textColor.b, 1.0f);
    }
}

// Predefined Styles
inline PanelStyle stylePultec() { 
    return { {0.1f, 0.2f, 0.4f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, Theme::MaterialType::WrinklePaint, true, "TUBE-P", "PROGRAM EQUALIZER" }; 
} 
inline PanelStyle styleSSL() { 
    return { {0.3f, 0.3f, 0.3f, 1.0f}, {0.1f, 0.1f, 0.1f, 1.0f}, Theme::MaterialType::Standard, true, "CONSOLE-E", "CHANNEL STRIP" }; 
} 
inline PanelStyle styleFET() { 
    return { {0.05f, 0.05f, 0.05f, 1.0f}, {0.9f, 0.9f, 0.9f, 1.0f}, Theme::MaterialType::Standard, true, "FET-76", "LIMITING AMPLIFIER" }; 
}

// =============================================================================
// KNOB - Round rotary control
// =============================================================================

inline void drawKnob(QuadBatcher& g, float x, float y, float size, 
                     Parameter& param, const char* label = nullptr) {
    float cx = x + size / 2;
    float cy = y + size / 2;
    float radius = size / 2 - 4;
    
    // Draw Ticks
    int numTicks = 11;
    float startAngle = -2.356f; // -135 deg
    float endAngle = 2.356f;   // +135 deg
    for (int i=0; i<numTicks; ++i) {
        float t = (float)i / (float)(numTicks - 1);
        float a = startAngle + t * (endAngle - startAngle);
        float tx1 = cx + std::sin(a) * (radius + 2);
        float ty1 = cy - std::cos(a) * (radius + 2);
        float tx2 = cx + std::sin(a) * (radius + 5);
        float ty2 = cy - std::cos(a) * (radius + 5);
        g.drawLine(tx1, ty1, tx2, ty2, 1.0f, 0.3f, 0.3f, 0.3f, 0.6f);
    }

    // Background circle (SDF Mode 4 for bevel)
    g.drawBeveledRect(x, y, size, size, size / 2, 2.0f, 0.15f, 0.15f, 0.18f, 1.0f);
    
    // Value arc
    float norm = param.getNormalizedValue();
    float angle = -135.0f + norm * 270.0f;
    float rad = (angle - 90.0f) * 3.14159f / 180.0f; // Adjusted for math orientation
    
    // Pointer line
    float px = cx + std::cos(rad) * radius * 0.7f;
    float py = cy + std::sin(rad) * radius * 0.7f;
    g.drawQuad(cx - 2, cy - 2, 4, 4, 0.9f, 0.9f, 0.9f, 1.0f);
    g.drawQuad(px - 3, py - 3, 6, 6, 0.3f, 0.8f, 0.5f, 1.0f);
    
    // Label below
    if (label) {
        g.drawText(label, x, y + size + 4, 10, 0.7f, 0.7f, 0.7f, 1.0f);
    }
    
    // Value text
    char valText[16];
    snprintf(valText, sizeof(valText), "%.1f", param.getValue());
    g.drawText(valText, cx - 12, cy - 5, 10, 1.0f, 1.0f, 1.0f, 1.0f);
}

// =============================================================================
// VERTICAL SLIDER - Fader style control
// =============================================================================

inline void drawVSlider(QuadBatcher& g, float x, float y, float w, float h,
                        Parameter& param, const char* label = nullptr) {
    // Track
    float trackW = 4;
    float trackX = x + (w - trackW) / 2;
    g.drawQuad(trackX, y, trackW, h, 0.1f, 0.1f, 0.12f, 1.0f);
    
    // Thumb position
    float norm = param.getNormalizedValue();
    float thumbY = y + h - 10 - norm * (h - 20);
    float thumbH = 16;
    
    // Thumb
    g.drawRoundedRect(x, thumbY, w, thumbH, 4.0f, 1.0f, 0.3f, 0.7f, 0.5f, 1.0f);
    
    // Label
    if (label) {
        g.drawText(label, x, y + h + 4, 10, 0.7f, 0.7f, 0.7f, 1.0f);
    }
}

// =============================================================================
// HORIZONTAL SLIDER
// =============================================================================

inline void drawHSlider(QuadBatcher& g, float x, float y, float w, float h,
                        Parameter& param, const char* label = nullptr) {
    // Track
    float trackH = 4;
    float trackY = y + (h - trackH) / 2;
    g.drawQuad(x, trackY, w, trackH, 0.1f, 0.1f, 0.12f, 1.0f);
    
    // Thumb
    float norm = param.getNormalizedValue();
    float thumbX = x + norm * (w - 16);
    g.drawRoundedRect(thumbX, y, 16, h, 4.0f, 1.0f, 0.3f, 0.7f, 0.5f, 1.0f);
    
    // Label
    if (label) {
        g.drawText(label, x, y + h + 2, 10, 0.7f, 0.7f, 0.7f, 1.0f);
    }
}

// =============================================================================
// LED INDICATOR
// =============================================================================

inline void drawLED(QuadBatcher& g, float x, float y, float size, 
                    bool on, LEDColor color = LED_GREEN) {
    float r, gr, b;
    getColorFromLED(color, r, gr, b);
    
    if (on) {
        // Glow effect
        g.drawRoundedRect(x - 2, y - 2, size + 4, size + 4, (size + 4) / 2, 
                          0.5f, r * 0.3f, gr * 0.3f, b * 0.3f, 0.5f);
        g.drawRoundedRect(x, y, size, size, size / 2, 0.5f, r, gr, b, 1.0f);
    } else {
        g.drawRoundedRect(x, y, size, size, size / 2, 0.5f, r * 0.3f, gr * 0.3f, b * 0.3f, 1.0f);
    }
}

// =============================================================================
// VU METER - Vertical level meter with peak hold
// =============================================================================

inline void drawVUMeter(QuadBatcher& g, float x, float y, float w, float h,
                         float level, float peak = -1.0f, bool isGR = false) {
    // Recessed Background
    g.drawRoundedRect(x - 2, y - 2, w + 4, h + 4, 3.0f, 0.5f, 0.05f, 0.05f, 0.06f, 1.0f);
    
    int numSegs = 14; 
    float gap = 2.0f;
    float segH = (h - (numSegs-1)*gap) / numSegs;
    
    for (int i = 0; i < numSegs; ++i) {
        float segPos = (float)i / (float)(numSegs - 1);
        bool lit = false;
        
        if (isGR) {
            lit = (level > 0.005f) && (segPos < level); 
        } else {
            float levelPos = 1.0f - segPos;
            lit = (level > 0.005f) && (levelPos < level);
        }
        
        Color col;
        if (isGR) {
            col = (segPos < 0.4f) ? Theme::LEDGreen : (segPos < 0.7f ? Theme::LEDYellow : Theme::LEDRed);
        } else {
            float levelPos = 1.0f - segPos;
            col = (levelPos < 0.7f) ? Theme::LEDGreen : (levelPos < 0.9f ? Theme::LEDYellow : Theme::LEDRed);
        }
        
        float alpha = lit ? 1.0f : 0.12f;
        g.drawRoundedRect(x, y + i*(segH + gap), w, segH, 1.5f, 0.5f, col.r, col.g, col.b, alpha);
        if (lit) {
             g.drawRoundedRect(x - 1, y + i*(segH + gap) - 1, w + 2, segH + 2, 1.5f, 1.0f, col.r, col.g, col.b, 0.2f);
        }
    }
}

// =============================================================================
// WAVEFORM DISPLAY
// =============================================================================

inline void drawWaveform(QuadBatcher& g, float x, float y, float w, float h,
                         const float* samples, int numSamples) {
    // Background
    g.drawQuad(x, y, w, h, 0.05f, 0.08f, 0.1f, 1.0f);
    g.drawRect(x, y, w, h, 1.0f, 0.2f, 0.2f, 0.25f, 1.0f);
    
    // Center line
    float cy = y + h / 2;
    g.drawQuad(x, cy - 0.5f, w, 1, 0.3f, 0.3f, 0.3f, 0.5f);
    
    if (!samples || numSamples <= 0) return;
    
    // Draw waveform
    float step = w / (float)numSamples;
    for (int i = 0; i < numSamples - 1; i++) {
        float x1 = x + i * step;
        float y1 = cy - samples[i] * (h / 2 - 2);
        float barH = std::abs(samples[i]) * (h / 2 - 2);
        g.drawQuad(x1, cy - barH, step, barH * 2, 0.3f, 0.8f, 0.5f, 0.8f);
    }
}

// =============================================================================
// SPECTRUM ANALYZER
// =============================================================================

inline void drawSpectrum(QuadBatcher& g, float x, float y, float w, float h,
                         const float* bins, int numBins) {
    // Background
    g.drawQuad(x, y, w, h, 0.03f, 0.05f, 0.08f, 1.0f);
    g.drawRect(x, y, w, h, 1.0f, 0.15f, 0.2f, 0.25f, 1.0f);
    
    if (!bins || numBins <= 0) return;
    
    float barW = w / (float)numBins;
    for (int i = 0; i < numBins; i++) {
        float barH = bins[i] * h;
        float barX = x + i * barW;
        float barY = y + h - barH;
        
        // Color gradient based on frequency
        float t = (float)i / numBins;
        g.drawQuad(barX, barY, barW - 1, barH, 
                   0.2f + t * 0.3f, 0.7f - t * 0.3f, 0.9f - t * 0.5f, 0.9f);
    }
}

// =============================================================================
// XY PAD - 2D control surface
// =============================================================================

inline void drawXYPad(QuadBatcher& g, float x, float y, float size,
                      Parameter& paramX, Parameter& paramY, const char* label = nullptr) {
    // Background
    g.drawQuad(x, y, size, size, 0.08f, 0.08f, 0.1f, 1.0f);
    g.drawRect(x, y, size, size, 1.0f, 0.25f, 0.25f, 0.3f, 1.0f);
    
    // Grid lines
    for (int i = 1; i < 4; i++) {
        float pos = size * i / 4.0f;
        g.drawQuad(x + pos, y, 1, size, 0.2f, 0.2f, 0.2f, 0.3f);
        g.drawQuad(x, y + pos, size, 1, 0.2f, 0.2f, 0.2f, 0.3f);
    }
    
    // Cursor position
    float normX = paramX.getNormalizedValue();
    float normY = 1.0f - paramY.getNormalizedValue(); // Invert Y
    float cx = x + normX * size;
    float cy = y + normY * size;
    
    // Crosshair
    g.drawQuad(cx - 8, cy, 16, 1, 0.5f, 0.9f, 0.6f, 0.8f);
    g.drawQuad(cx, cy - 8, 1, 16, 0.5f, 0.9f, 0.6f, 0.8f);
    
    // Cursor dot
    g.drawRoundedRect(cx - 5, cy - 5, 10, 10, 5.0f, 1.0f, 0.4f, 0.9f, 0.5f, 1.0f);
    
    if (label) {
        g.drawText(label, x, y + size + 4, 10, 0.7f, 0.7f, 0.7f, 1.0f);
    }
}

// =============================================================================
// TOGGLE BUTTON
// =============================================================================

inline void drawToggle(QuadBatcher& g, float x, float y, float w, float h,
                       bool state, const char* label = nullptr) {
    if (state) {
        g.drawRoundedRect(x, y, w, h, 4.0f, 1.0f, 0.3f, 0.7f, 0.4f, 1.0f);
    } else {
        g.drawRoundedRect(x, y, w, h, 4.0f, 1.0f, 0.2f, 0.2f, 0.22f, 1.0f);
    }
    g.drawRect(x, y, w, h, 1.0f, 0.4f, 0.4f, 0.45f, 1.0f);
    
    if (label) {
        g.drawText(label, x + 4, y + (h - 12) / 2, 11, 1.0f, 1.0f, 1.0f, 1.0f);
    }
}

// =============================================================================
// TEXT LABEL
// =============================================================================

inline void drawLabel(QuadBatcher& g, float x, float y, const char* text,
                      float size = 12, float r = 0.8f, float gr = 0.8f, float b = 0.8f) {
    g.drawText(text, x, y, size, r, gr, b, 1.0f);
}

// =============================================================================
// GROUP BOX - Container with title
// =============================================================================

inline void drawGroupBox(QuadBatcher& g, float x, float y, float w, float h,
                         const char* title) {
    // Border
    g.drawRect(x, y + 8, w, h - 8, 1.0f, 0.25f, 0.25f, 0.3f, 1.0f);
    
    // Title background
    float titleW = 60; // Approximate
    g.drawQuad(x + 8, y, titleW, 16, 0.12f, 0.12f, 0.14f, 1.0f);
    
    // Title text
    if (title) {
        g.drawText(title, x + 12, y + 2, 11, 0.7f, 0.7f, 0.7f, 1.0f);
    }
}

// =============================================================================
// PIANO KEYBOARD (Mini)
// =============================================================================

inline void drawMiniKeyboard(QuadBatcher& g, float x, float y, float w, float h,
                             int startNote = 36, int numKeys = 24) {
    float whiteW = w / (numKeys * 7 / 12); // Approximate white key count
    float blackW = whiteW * 0.6f;
    float blackH = h * 0.6f;
    
    // Draw white keys
    float wx = x;
    for (int i = 0; i < numKeys; i++) {
        int note = (startNote + i) % 12;
        bool isBlack = (note == 1 || note == 3 || note == 6 || note == 8 || note == 10);
        if (!isBlack) {
            g.drawQuad(wx, y, whiteW - 1, h, 0.95f, 0.95f, 0.9f, 1.0f);
            g.drawRect(wx, y, whiteW - 1, h, 1.0f, 0.3f, 0.3f, 0.3f, 1.0f);
            wx += whiteW;
        }
    }
    
    // Draw black keys
    wx = x + whiteW * 0.7f;
    for (int i = 0; i < numKeys; i++) {
        int note = (startNote + i) % 12;
        bool isBlack = (note == 1 || note == 3 || note == 6 || note == 8 || note == 10);
        if (isBlack) {
            g.drawQuad(wx, y, blackW, blackH, 0.1f, 0.1f, 0.1f, 1.0f);
        }
        if (!isBlack && note != 4 && note != 11) {
            wx += whiteW;
        }
    }
}

// =============================================================================
// ENVELOPE DISPLAY (ADSR)
// =============================================================================

inline void drawEnvelope(QuadBatcher& g, float x, float y, float w, float h,
                         float attack, float decay, float sustain, float release) {
    // Background
    g.drawQuad(x, y, w, h, 0.05f, 0.05f, 0.07f, 1.0f);
    g.drawRect(x, y, w, h, 1.0f, 0.2f, 0.2f, 0.25f, 1.0f);
    
    // Normalize times (assume max 2 sec each)
    float aW = (attack / 2.0f) * (w * 0.25f);
    float dW = (decay / 2.0f) * (w * 0.25f);
    float rW = (release / 2.0f) * (w * 0.25f);
    float sW = w - aW - dW - rW;
    
    float baseline = y + h - 2;
    float peak = y + 2;
    float sustainY = y + h - sustain * (h - 4);
    
    // Attack line
    g.drawQuad(x, baseline, 2, -(h - 4), 0.3f, 0.8f, 0.5f, 0.8f);
    // Decay line (peak to sustain)
    // Sustain line
    // Release line
    
    // Simplified: just draw points
    g.drawRoundedRect(x + aW - 3, peak - 3, 6, 6, 3.0f, 1.0f, 0.4f, 0.9f, 0.5f, 1.0f);
    g.drawRoundedRect(x + aW + dW - 3, sustainY - 3, 6, 6, 3.0f, 1.0f, 0.4f, 0.9f, 0.5f, 1.0f);
}

} // namespace SDK
} // namespace Beam

#endif // BEAM_WIDGETS_HPP
