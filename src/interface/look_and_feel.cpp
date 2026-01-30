#include "look_and_feel.hpp"
#include "button.hpp"
#include "slider.hpp"
#include "knob.hpp"
#include "meter.hpp"
#include "vu_meter.hpp"
#include "spectrum_module.hpp"
#include "slider_modular.hpp"
#include "audio_module.hpp"
#include "../utilities/flux_audio_utils.hpp"
#include <algorithm>
#include <cmath>
#include <string>

namespace Beam {

// Brand Colors (Normalized)
static const float BRAND_BLACK[3] = {0.05f, 0.05f, 0.06f};
static const float BRAND_WHITE[3] = {1.0f, 1.0f, 1.0f};
static const float BRAND_EMERALD[3] = {0.13f, 0.62f, 0.42f}; // #219e6c
static const float BRAND_RED[3] = {0.56f, 0.03f, 0.03f};     // #8f0707

void LookAndFeel::drawButtonBackground(QuadBatcher& g, Button& button, 
                                     bool isMouseOver, bool isButtonDown) {}

void LookAndFeel::drawButtonText(QuadBatcher& g, Button& button, 
                                bool isMouseOver, bool isButtonDown) {}

void DefaultLookAndFeel::drawButtonBackground(QuadBatcher& g, Button& button, 
                                            bool isMouseOver, bool isButtonDown) {
    auto bounds = button.getBounds();
    bool toggled = button.getToggleState();
    
    // Base Colors
    float r = 0.15f, gr = 0.15f, b = 0.16f; // Dark Grey Base
    float alpha = 1.0f;

    if (toggled) {
        r = BRAND_EMERALD[0]; gr = BRAND_EMERALD[1]; b = BRAND_EMERALD[2];
    } else if (isButtonDown) {
        r = 0.1f; gr = 0.1f; b = 0.1f;
    } else if (isMouseOver) {
        r = 0.25f; gr = 0.25f; b = 0.26f;
    }

    // Shadow / Glow
    g.drawQuad(bounds.x + 2, bounds.y + 2, bounds.w, bounds.h, 0.0f, 0.0f, 0.0f, 0.3f);

    // Main Body Gradient
    g.drawRoundedGradientRect(bounds.x, bounds.y, bounds.w, bounds.h, 4.0f, 0.5f,
                             r + 0.1f, gr + 0.1f, b + 0.1f, 1.0f,  // Top Light
                             r - 0.05f, gr - 0.05f, b - 0.05f, 1.0f); // Bottom Dark

    // Highlight Stroke
    g.drawRoundedRect(bounds.x, bounds.y, bounds.w, bounds.h, 4.0f, 1.0f, 
                      1.0f, 1.0f, 1.0f, (isMouseOver ? 0.2f : 0.1f));
}

void DefaultLookAndFeel::drawButtonText(QuadBatcher& g, Button& button, 
                                      bool isMouseOver, bool isButtonDown) {
    auto bounds = button.getBounds();
    const auto& text = button.getButtonText();
    float tw = AudioUtils::calculateTextWidth(text, 12.0f);
    
    float textY = bounds.y + (bounds.h - 12)/2;
    if (isButtonDown) textY += 1.0f; // Press effect

    // Text Shadow
    g.drawText(text, bounds.x + (bounds.w - tw)/2 + 1, textY + 1, 12.0f, 0.0f, 0.0f, 0.0f, 0.5f);
    // Text
    g.drawText(text, bounds.x + (bounds.w - tw)/2, textY, 12.0f, 
               BRAND_WHITE[0], BRAND_WHITE[1], BRAND_WHITE[2], (button.isEnabled() ? 1.0f : 0.4f));
}

void DefaultLookAndFeel::drawSliderBackground(QuadBatcher& g, Slider& slider, 
                                            float sliderPos, float rotaryStartAngle, float rotaryEndAngle) {
    auto bounds = slider.getBounds();
    // Groove
    g.drawRoundedRect(bounds.x, bounds.y, bounds.w, bounds.h, 3.0f, 0.5f, 0.08f, 0.08f, 0.08f, 1.0f);
    // Inner Shadow
    g.drawRoundedRect(bounds.x + 1, bounds.y + 1, bounds.w - 2, bounds.h - 2, 3.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.3f);
}

void DefaultLookAndFeel::drawSliderPointer(QuadBatcher& g, Slider& slider, 
                                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle) {
    auto bounds = slider.getBounds();
    if (slider.getSliderStyle() == SliderStyle::LinearHorizontal) {
        g.drawRoundedGradientRect(bounds.x, bounds.y, bounds.w * sliderPos, bounds.h, 2.0f, 0.5f,
                                 BRAND_EMERALD[0], BRAND_EMERALD[1], BRAND_EMERALD[2], 1.0f,
                                 BRAND_EMERALD[0]*0.5f, BRAND_EMERALD[1]*0.5f, BRAND_EMERALD[2]*0.5f, 1.0f);
    } else if (slider.getSliderStyle() == SliderStyle::LinearVertical) {
        float h = bounds.h * sliderPos;
        // Fader Cap
        float capH = 20.0f;
        float capY = bounds.y + bounds.h - h - (capH/2);
        if (capY < bounds.y) capY = bounds.y;
        if (capY > bounds.y + bounds.h - capH) capY = bounds.y + bounds.h - capH;

        // Fill
        g.drawRoundedGradientRect(bounds.x, bounds.y + bounds.h - h, bounds.w, h, 2.0f, 0.5f,
                                 BRAND_EMERALD[0], BRAND_EMERALD[1], BRAND_EMERALD[2], 1.0f,
                                 BRAND_EMERALD[0]*0.4f, BRAND_EMERALD[1]*0.4f, BRAND_EMERALD[2]*0.4f, 1.0f);
        
        // Cap
        g.drawRoundedGradientRect(bounds.x - 2, capY, bounds.w + 4, capH, 3.0f, 0.5f,
                                 0.3f, 0.3f, 0.35f, 1.0f,
                                 0.1f, 0.1f, 0.12f, 1.0f);
        g.drawQuad(bounds.x - 2, capY + capH/2 - 1, bounds.w + 4, 2, 0.0f, 0.0f, 0.0f, 0.5f); // Grip line
    }
}

void DefaultLookAndFeel::drawKnob(QuadBatcher& g, Knob& knob, float sliderPos) {
    auto bounds = knob.getBounds();
    float cx = bounds.x + bounds.w * 0.5f;
    float cy = bounds.y + bounds.h * 0.5f;
    float radius = (std::min)(bounds.w, bounds.h) * 0.4f;

    // Outer Shadow
    g.drawQuad(cx - radius - 2, cy - radius + 2, radius * 2 + 4, radius * 2 + 4, 0.0f, 0.0f, 0.0f, 0.4f);

    // Knob Body (Metallic Gradient)
    g.drawRoundedGradientRect(cx - radius, cy - radius, radius * 2, radius * 2, radius, 0.5f,
                             0.25f, 0.25f, 0.27f, 1.0f, 
                             0.1f, 0.1f, 0.12f, 1.0f);
    
    // Top Shine
    g.drawRoundedGradientRect(cx - radius, cy - radius, radius * 2, radius, radius/2, 0.5f,
                             1.0f, 1.0f, 1.0f, 0.1f, 
                             1.0f, 1.0f, 1.0f, 0.0f);

    // Indicator
    float angle = -135.0f + sliderPos * 270.0f; 
    float radAngle = (angle - 90.0f) * 3.14159f / 180.0f;
    float indR = radius * 0.7f;
    
    // Indicator Glow
    g.drawLine(cx, cy, cx + std::cos(radAngle)*indR, cy + std::sin(radAngle)*indR, 4.0f, 
               BRAND_EMERALD[0], BRAND_EMERALD[1], BRAND_EMERALD[2], 0.3f);
    // Indicator Line
    g.drawLine(cx, cy, cx + std::cos(radAngle)*indR, cy + std::sin(radAngle)*indR, 2.0f, 
               BRAND_EMERALD[0], BRAND_EMERALD[1], BRAND_EMERALD[2], 1.0f);
    
    // Label
    g.drawText(knob.getLabel(), bounds.x, bounds.y - 12, 10, 0.8f, 0.8f, 0.8f, 1.0f);
    
    // Value Ring
    float startRad = (-135.0f - 90.0f) * 3.14159f / 180.0f;
    float endRad = radAngle;
    // (Note: QuadBatcher doesn't support arcs yet, so skipping ring for now)

    char valStr[16];
    snprintf(valStr, 16, "%.2f", knob.getValue());
    g.drawText(valStr, bounds.x + (bounds.w - AudioUtils::calculateTextWidth(valStr, 9))/2, bounds.y + bounds.h + 2, 9, 0.6f, 0.6f, 0.6f, 1.0f);
}

void DefaultLookAndFeel::drawLuminousMeter(QuadBatcher& g, LuminousMeter& meter, float level, float peak) {
    auto bounds = meter.getBounds();
    // Background Dark Track
    g.drawRoundedRect(bounds.x, bounds.y, bounds.w, bounds.h, 2.0f, 0.5f, 0.05f, 0.05f, 0.05f, 1.0f);
    
    if (meter.getOrientation() == LuminousMeter::Orientation::Vertical) {
        int segments = 30;
        float segHeight = bounds.h / segments;
        int activeSegs = (int)(level * segments);
        
        for (int i = 0; i < segments; ++i) {
            float y = bounds.y + bounds.h - (i + 1) * segHeight;
            bool active = i < activeSegs;
            
            float r, gr, b;
            if (i > segments * 0.9f) { r = 0.9f; gr = 0.1f; b = 0.1f; } // Clip
            else if (i > segments * 0.75f) { r = 0.9f; gr = 0.8f; b = 0.1f; } // Warning
            else { r = BRAND_EMERALD[0]; gr = BRAND_EMERALD[1]; b = BRAND_EMERALD[2]; } // Normal

            if (!active) { r *= 0.2f; gr *= 0.2f; b *= 0.2f; } // Dimmed
            
            g.drawRoundedRect(bounds.x + 1, y + 1, bounds.w - 2, segHeight - 2, 1.0f, 0.5f, r, gr, b, 1.0f);
        }
    }
}

void DefaultLookAndFeel::drawVUMeter(QuadBatcher& g, VUMeter& meter, float level) {
    auto b = meter.getBounds();
    
    // 1. Bezel (Metallic)
    g.drawRoundedGradientRect(b.x, b.y, b.w, b.h, 6.0f, 0.5f, 
                             0.3f, 0.3f, 0.32f, 1.0f, 
                             0.1f, 0.1f, 0.12f, 1.0f);
    
    // 2. Glass / Face
    Rect face = {b.x + 4, b.y + 4, b.w - 8, b.h - 8};
    g.drawRoundedGradientRect(face.x, face.y, face.w, face.h, 4.0f, 0.5f, 
                             0.95f, 0.93f, 0.85f, 1.0f,  // Warm White Top
                             0.85f, 0.82f, 0.70f, 1.0f); // Aged Paper Bottom

    float cx = face.x + face.w * 0.5f;
    float pivotY = face.y + face.h * 1.5f; // Pivot far below
    float needleLen = face.h * 1.3f;
    
    // 3. Scale Markings (Arc approximation)
    float startAng = -0.5f;
    float endAng = 0.5f;
    for (float t = 0; t <= 1.0f; t += 0.1f) {
        float a = startAng + t * (endAng - startAng);
        float r1 = needleLen - 10.0f;
        float r2 = needleLen - 4.0f;
        float x1 = cx + std::sin(a) * r1;
        float y1 = pivotY - std::cos(a) * r1;
        float x2 = cx + std::sin(a) * r2;
        float y2 = pivotY - std::cos(a) * r2;
        
        bool isRed = (t > 0.75f);
        g.drawLine(x1, y1, x2, y2, (t == 0.5f || t == 0.0f || t == 1.0f) ? 2.0f : 1.0f, 
                   isRed ? 0.8f : 0.2f, isRed ? 0.1f : 0.2f, isRed ? 0.1f : 0.2f, 0.8f);
    }

    // 4. VU Label
    g.drawText("VU", cx - 8, face.y + face.h * 0.6f, 12, 0.2f, 0.2f, 0.2f, 0.8f);

    // 5. Needle
    // Map level 0..1.2 to angle
    // -20dB to +3dB. 0 VU is usually at ~0.707 (sine rms) of digital max? 
    // Let's assume input 'level' is linear amplitude 0..1
    // Log scale is better for VU, but linear is fine for visuals if 'level' is already pre-processed.
    // Assuming level is linear 0..1 where 1 is clip. 0.7 is 0 VU.
    
    float vuAngle = startAng + (std::clamp(level, 0.0f, 1.2f) / 1.2f) * (endAng - startAng);
    
    float nx = cx + std::sin(vuAngle) * needleLen;
    float ny = pivotY - std::cos(vuAngle) * needleLen;
    
    // Shadow
    g.drawLine(cx + 2, pivotY + 2, nx + 2, ny + 2, 1.5f, 0.0f, 0.0f, 0.0f, 0.2f);
    // Needle
    g.drawLine(cx, pivotY, nx, ny, 1.5f, 0.8f, 0.1f, 0.1f, 1.0f);

    // 6. Glass Reflection (Gloss)
    g.drawRoundedGradientRect(face.x, face.y, face.w, face.h * 0.4f, 4.0f, 0.5f,
                             1.0f, 1.0f, 1.0f, 0.3f,
                             1.0f, 1.0f, 1.0f, 0.0f);
}

void DefaultLookAndFeel::drawSpectrumAnalyzer(QuadBatcher& g, SpectrumModule& spectrum) {
    auto bounds = spectrum.getBounds();
    g.drawRoundedRect(bounds.x, bounds.y, bounds.w, bounds.h, 6.0f, 1.0f, 0.1f, 0.1f, 0.12f, 1.0f);
    
    // Header
    g.drawRoundedGradientRect(bounds.x, bounds.y, bounds.w, 24, 6.0f, 0.5f, 
                             0.15f, 0.15f, 0.17f, 1.0f, 
                             0.1f, 0.1f, 0.12f, 1.0f);
    g.drawText("SPECTRUM", bounds.x + 8, bounds.y + 6, 10, 0.7f, 0.7f, 0.7f, 1.0f);
    
    // Grid
    g.drawRect(bounds.x + 4, bounds.y + 28, bounds.w - 8, bounds.h - 32, 1.0f, 0.0f, 0.0f, 0.0f, 0.5f);
}

void DefaultLookAndFeel::drawAudioModule(QuadBatcher& g, AudioModule& module) {
    auto bounds = module.getBounds();
    std::string name = module.getName();
    bool isMaster = (name == "Master" || name == "MASTER");

    // Body
    g.drawRoundedRect(bounds.x, bounds.y, bounds.w, bounds.h, 8.0f, 1.0f, 0.12f, 0.12f, 0.13f, 1.0f);
    
    // Header
    float hr = 0.2f, hg = 0.2f, hb = 0.22f;
    if (isMaster) { hr = 0.3f; hg = 0.1f; hb = 0.1f; } // Dark Red for Master
    
    g.drawRoundedGradientRect(bounds.x, bounds.y, bounds.w, 24, 8.0f, 0.5f, 
                             hr + 0.1f, hg + 0.1f, hb + 0.1f, 1.0f, 
                             hr, hg, hb, 1.0f);

    g.drawText(name, bounds.x + 10, bounds.y + 6, 11, 1.0f, 1.0f, 1.0f, 0.9f);
    
    // Close Button
    if (!isMaster) {
        float bx = bounds.x + bounds.w - 20;
        float by = bounds.y + 6;
        g.drawRoundedRect(bx, by, 14, 14, 4.0f, 0.5f, 0.8f, 0.2f, 0.2f, 0.8f);
        g.drawText("x", bx + 4, by + 1, 10, 1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    // Content Area
    g.drawRoundedRect(bounds.x + 4, bounds.y + 28, bounds.w - 8, bounds.h - 32, 4.0f, 1.0f, 0.08f, 0.08f, 0.09f, 1.0f);
}

void DefaultLookAndFeel::drawModularSlider(QuadBatcher& g, ModularSlider& slider) {
    auto b = slider.getBounds();
    float val = slider.getParameter() ? slider.getParameter()->getNormalizedValue() : 0.5f;
    
    g.drawText(slider.getLabel(), b.x, b.y, 9, 0.7f, 0.7f, 0.7f, 1.0f);
    
    // Track
    float trackY = b.y + 14;
    g.drawRoundedRect(b.x, trackY, b.w, 4, 2.0f, 0.5f, 0.05f, 0.05f, 0.05f, 1.0f);
    
    // Fill
    g.drawRoundedGradientRect(b.x, trackY, b.w * val, 4, 2.0f, 0.5f, 
                             BRAND_EMERALD[0], BRAND_EMERALD[1], BRAND_EMERALD[2], 1.0f, 
                             BRAND_EMERALD[0]*0.5f, BRAND_EMERALD[1]*0.5f, BRAND_EMERALD[2]*0.5f, 1.0f);
    
    // Handle
    g.drawRoundedRect(b.x + val * (b.w - 6), trackY - 4, 6, 12, 2.0f, 0.5f, 0.9f, 0.9f, 0.9f, 1.0f);
}

void ModernLookAndFeel::drawButtonBackground(QuadBatcher& g, Button& button, 
                                           bool isMouseOver, bool isButtonDown) {
    auto bounds = button.getBounds();
    bool toggled = button.getToggleState();
    
    // Flat Modern Look
    float r = 0.12f, gr = 0.12f, b = 0.14f; // Very dark blue-grey
    if (toggled) { r = 0.0f; gr = 0.6f; b = 0.8f; } // Neon Blue
    else if (isButtonDown) { r = 0.08f; gr = 0.08f; b = 0.1f; }
    else if (isMouseOver) { r = 0.18f; gr = 0.18f; b = 0.22f; }

    g.drawRoundedRect(bounds.x, bounds.y, bounds.w, bounds.h, 4.0f, 0.5f, r, gr, b, 1.0f);
    
    // Subtle border
    if (isMouseOver && !toggled)
        g.drawRoundedRect(bounds.x, bounds.y, bounds.w, bounds.h, 4.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.1f);
}

void ModernLookAndFeel::drawSliderBackground(QuadBatcher& g, Slider& slider, 
                                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle) {
    auto bounds = slider.getBounds();
    // Dark track
    g.drawRoundedRect(bounds.x, bounds.y, bounds.w, bounds.h, 2.0f, 0.5f, 0.05f, 0.05f, 0.07f, 1.0f);
}

void ModernLookAndFeel::drawSliderPointer(QuadBatcher& g, Slider& slider, 
                                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle) {
    auto bounds = slider.getBounds();
    // Neon fill
    float w = bounds.w * sliderPos;
    g.drawRoundedRect(bounds.x, bounds.y, w, bounds.h, 2.0f, 0.5f, 0.0f, 0.8f, 1.0f, 1.0f); // Cyan
}

void ModernLookAndFeel::drawKnob(QuadBatcher& g, Knob& knob, float sliderPos) {
    auto bounds = knob.getBounds();
    float cx = bounds.x + bounds.w * 0.5f;
    float cy = bounds.y + bounds.h * 0.5f;
    float radius = (std::min)(bounds.w, bounds.h) * 0.4f;

    // Vector Arc
    float startAng = -135.0f * 3.14159f / 180.0f;
    float endAng = 135.0f * 3.14159f / 180.0f;
    float currentAng = startAng + sliderPos * (endAng - startAng);

    // Track (Grey)
    // QuadBatcher doesn't support arcs natively yet, simulating with points?
    // For now, draw simple rects or circle approx if I had circle primitive.
    // I'll stick to the "Modern" flat style which might just be a circle.
    
    // Background Circle
    g.drawRoundedRect(cx - radius, cy - radius, radius*2, radius*2, radius, 0.5f, 0.1f, 0.1f, 0.12f, 1.0f);
    
    // Value Indicator (Line)
    float ix = cx + std::cos(currentAng) * radius * 0.8f;
    float iy = cy + std::sin(currentAng) * radius * 0.8f;
    g.drawLine(cx, cy, ix, iy, 3.0f, 0.0f, 0.8f, 1.0f, 1.0f); // Cyan line
    
    // Label
    g.drawText(knob.getLabel(), bounds.x, bounds.y - 12, 10, 0.6f, 0.6f, 0.7f, 1.0f);
}

void ModernLookAndFeel::drawLuminousMeter(QuadBatcher& g, LuminousMeter& meter, float level, float peak) {
    auto b = meter.getBounds();
    g.drawRoundedRect(b.x, b.y, b.w, b.h, 2.0f, 0.5f, 0.02f, 0.02f, 0.03f, 1.0f);
    
    float fillW = b.w * std::clamp(level, 0.0f, 1.0f);
    // Gradient fill: Green -> Yellow -> Red
    float r = level > 0.8f ? 1.0f : 0.0f;
    float gr = level < 0.9f ? 1.0f : 0.0f;
    
    g.drawRoundedRect(b.x, b.y, fillW, b.h, 2.0f, 0.5f, r, gr, 0.2f, 0.8f);
}

void ModernLookAndFeel::drawVUMeter(QuadBatcher& g, VUMeter& m, float l) { DefaultLookAndFeel::drawVUMeter(g, m, l); }
void ModernLookAndFeel::drawSpectrumAnalyzer(QuadBatcher& g, SpectrumModule& s) { DefaultLookAndFeel::drawSpectrumAnalyzer(g, s); }
void ModernLookAndFeel::drawAudioModule(QuadBatcher& g, AudioModule& module) {
    auto b = module.getBounds();
    // Modern Dark Card
    g.drawRoundedRect(b.x, b.y, b.w, b.h, 6.0f, 1.0f, 0.08f, 0.08f, 0.09f, 1.0f);
    // Header
    g.drawRoundedRect(b.x, b.y, b.w, 24, 6.0f, 0.5f, 0.15f, 0.15f, 0.18f, 1.0f);
    g.drawText(module.getName(), b.x + 10, b.y + 6, 11, 0.9f, 0.9f, 0.95f, 1.0f);
    
    // Border highlight
    g.drawRoundedRect(b.x, b.y, b.w, b.h, 6.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.05f);
}

void ModernLookAndFeel::drawModularSlider(QuadBatcher& g, ModularSlider& slider) {
    auto b = slider.getBounds();
    float val = slider.getParameter() ? slider.getParameter()->getNormalizedValue() : 0.5f;
    
    g.drawText(slider.getLabel(), b.x, b.y, 9, 0.5f, 0.5f, 0.6f, 1.0f);
    
    float trackY = b.y + 14;
    g.drawRoundedRect(b.x, trackY, b.w, 4, 2.0f, 0.5f, 0.02f, 0.02f, 0.03f, 1.0f);
    g.drawRoundedRect(b.x, trackY, b.w * val, 4, 2.0f, 0.5f, 0.0f, 0.7f, 0.9f, 1.0f); // Cyan fill
}

} // namespace Beam
