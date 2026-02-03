#include "interface/core/look_and_feel.hpp"
#include "interface/core/theme.hpp"
#include "interface/widgets/button.hpp"
#include "interface/widgets/slider.hpp"
#include "interface/widgets/knob.hpp"
#include "interface/widgets/meter.hpp"
#include "interface/widgets/vu_meter.hpp"
#include "interface/editors/spectrum_module.hpp"
#include "interface/modules/audio_module.hpp"
#include "engine/dsp/flux_audio_utils.hpp"
#include <algorithm>
#include <cmath>
#include <string>

namespace Beam {

// Brand Colors (Normalized)
static const float BRAND_BLACK[3] = {Theme::Black.r, Theme::Black.g, Theme::Black.b};
static const float BRAND_WHITE[3] = {Theme::White.r, Theme::White.g, Theme::White.b};
static const float BRAND_EMERALD[3] = {Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b}; // #219e6c
static const float BRAND_RED[3] = {Theme::Red.r, Theme::Red.g, Theme::Red.b};     // #8f0707

void LookAndFeel::drawButtonBackground(QuadBatcher& g, Button& button, 
                                     bool isMouseOver, bool isButtonDown) {}

void LookAndFeel::drawButtonText(QuadBatcher& g, Button& button, 
                                bool isMouseOver, bool isButtonDown) {}

void DefaultLookAndFeel::drawButtonBackground(QuadBatcher& g, Button& button, 
                                            bool isMouseOver, bool isButtonDown) {
    auto bounds = button.getBounds();
    bool toggled = button.getToggleState();
    
    // Base Colors - Using Bakelite/Console palette
    Color base = Theme::Bakelite;
    if (toggled) base = Theme::Emerald;
    else if (isButtonDown) base = Theme::Bakelite.darker(0.05f);
    else if (isMouseOver) base = Theme::Bakelite.brighter(0.05f);

    // Physical Shadow
    if (!isButtonDown) {
        g.drawQuad(1, 2, bounds.w, bounds.h, 0.0f, 0.0f, 0.0f, 0.4f);
    }

    // Beveled Body
    g.drawBeveledRect(0, (isButtonDown ? 1.0f : 0.0f), bounds.w, bounds.h, 2.0f, 0.5f,
                      base.r, base.g, base.b, 1.0f);

    // Indicator Light for toggled state
    if (toggled) {
        float ledSize = 4.0f;
        g.drawRoundedRect(4, 4, ledSize, ledSize, ledSize*0.5f, 0.5f, 
                          Theme::LEDGreen.r, Theme::LEDGreen.g, Theme::LEDGreen.b, 1.0f);
    }
}

void DefaultLookAndFeel::drawButtonText(QuadBatcher& g, Button& button, 
                                      bool isMouseOver, bool isButtonDown) {
    auto bounds = button.getBounds();
    const auto& text = button.getButtonText();
    float tw = AudioUtils::calculateTextWidth(text, 12.0f);
    
    float textY = (bounds.h - 12)/2;
    if (isButtonDown) textY += 1.0f; // Press effect

    // Text Shadow
    g.drawText(text, (bounds.w - tw)/2 + 1, textY + 1, 12.0f, 0.0f, 0.0f, 0.0f, 0.5f);
    // Text
    g.drawText(text, (bounds.w - tw)/2, textY, 12.0f, 
               BRAND_WHITE[0], BRAND_WHITE[1], BRAND_WHITE[2], (button.isEnabled() ? 1.0f : 0.4f));
}

void DefaultLookAndFeel::drawSliderBackground(QuadBatcher& g, Slider& slider, 
                                            float sliderPos, float rotaryStartAngle, float rotaryEndAngle) {
    auto bounds = slider.getBounds();
    
    if (slider.getSliderStyle() == SliderStyle::LinearHorizontal) {
        // Slot Track (Centered Vertically)
        float trackH = 6.0f;
        float trackY = (bounds.h - trackH) / 2.0f;
        
        // Beveled Slot
        g.drawRoundedRect(0, trackY, bounds.w, trackH, 3.0f, 0.5f, 0.05f, 0.05f, 0.05f, 1.0f);
        g.drawRoundedRect(1, trackY + 1, bounds.w - 2, trackH - 2, 2.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.8f); // Deep Shadow
    } 
    else if (slider.getSliderStyle() == SliderStyle::LinearVertical) {
        // Slot Track (Centered Horizontally)
        float trackW = 6.0f;
        float trackX = (bounds.w - trackW) / 2.0f;
        
        g.drawRoundedRect(trackX, 0, trackW, bounds.h, 3.0f, 0.5f, 0.05f, 0.05f, 0.05f, 1.0f);
        g.drawRoundedRect(trackX + 1, 1, trackW - 2, bounds.h - 2, 2.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.8f);
    }
}

void DefaultLookAndFeel::drawSliderPointer(QuadBatcher& g, Slider& slider, 
                                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle) {
    auto bounds = slider.getBounds();
    if (slider.getSliderStyle() == SliderStyle::LinearHorizontal) {
        float capW = 12.0f; // Width of the handle
        float capH = bounds.h - 2; // Almost full height
        float trackLen = bounds.w - capW;
        float capX = trackLen * sliderPos;
        float capY = 1.0f;

        // Shadow
        g.drawQuad(capX + 2, capY + 4, capW, capH, 0.0f, 0.0f, 0.0f, 0.5f);

        // Cap Body (Bakelite)
        g.drawBeveledRect(capX, capY, capW, capH, 2.0f, 0.5f, Theme::Bakelite.r, Theme::Bakelite.g, Theme::Bakelite.b, 1.0f);
        
        // Indicator Line (White/Aluminum)
        g.drawQuad(capX + capW/2 - 1, capY + 2, 2, capH - 4, 0.9f, 0.9f, 0.9f, 0.9f);
    } else if (slider.getSliderStyle() == SliderStyle::LinearVertical) {
        float h = bounds.h * sliderPos;
        // Fader Cap (Bakelite with Aluminum stripe)
        float capH = 24.0f;
        float capY = bounds.h - h - (capH/2);
        capY = std::clamp(capY, 0.0f, bounds.h - capH);

        // Track Fill (Emerald Glow)
        g.drawRoundedRect(bounds.w*0.4f, bounds.h - h, bounds.w * 0.2f, h, 1.0f, 0.5f,
                          Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b, 0.6f);
        
        // Shadow
        g.drawQuad(-1, capY + 2, bounds.w + 2, capH, 0.0f, 0.0f, 0.0f, 0.4f);

        // Cap Body
        g.drawBeveledRect(-2, capY, bounds.w + 4, capH, 3.0f, 0.5f,
                          Theme::Bakelite.r, Theme::Bakelite.g, Theme::Bakelite.b, 1.0f);
        
        // Aluminum Grip Stripe
        g.drawQuad(-2, capY + capH/2 - 1, bounds.w + 4, 2, 
                   Theme::Aluminum.r, Theme::Aluminum.g, Theme::Aluminum.b, 0.8f);
    }
}

void DefaultLookAndFeel::drawKnob(QuadBatcher& g, Knob& knob, float sliderPos) {
    auto bounds = knob.getBounds();
    float w = bounds.w;
    float h = bounds.h;
    
    // Center knob in top area
    float knobAreaH = 50.0f; // Match kh from RackUnitUI
    float cx = w * 0.5f;
    float cy = knobAreaH * 0.5f;
    float radius = 20.0f; // Consistent radius

    // 1. Knob Shadow (Offset for depth)
    g.drawRoundedRect(cx - radius + 1, cy - radius + 2, radius * 2, radius * 2, radius, 0.5f, 0.0f, 0.0f, 0.0f, 0.5f);

    // 2. Main Body (Bakelite)
    g.drawRoundedGradientRect(cx - radius, cy - radius, radius * 2, radius * 2, radius, 0.5f,
                             Theme::Bakelite.brighter(0.05f).r, Theme::Bakelite.brighter(0.05f).g, Theme::Bakelite.brighter(0.05f).b, 1.0f, 
                             Theme::Bakelite.darker(0.05f).r, Theme::Bakelite.darker(0.05f).g, Theme::Bakelite.darker(0.05f).b, 1.0f);
    
    // 3. Top Grip (Aluminum ring)
    float gripR = radius * 0.85f;
    g.drawRoundedRect(cx - gripR, cy - gripR, gripR * 2, gripR * 2, gripR, 0.5f, 
                      Theme::Aluminum.r, Theme::Aluminum.g, Theme::Aluminum.b, 0.2f);

    // 4. Indicator (Emerald Glowing Line)
    float angle = -135.0f + sliderPos * 270.0f; 
    float radAngle = (angle - 90.0f) * 3.14159f / 180.0f;
    float indR1 = radius * 0.4f;
    float indR2 = radius * 0.95f;
    
    float x1 = cx + std::cos(radAngle) * indR1;
    float y1 = cy + std::sin(radAngle) * indR1;
    float x2 = cx + std::cos(radAngle) * indR2;
    float y2 = cy + std::sin(radAngle) * indR2;

    // Indicator Glow
    g.drawLine(x1, y1, x2, y2, 4.0f, Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b, 0.3f);
    // Indicator Line
    g.drawLine(x1, y1, x2, y2, 2.0f, Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b, 1.0f);
    
    // 5. Label (Engraved)
    // Label is now handled by Knob's TextElement child component, but we'll keep this 
    // for fallback or simple knobs that don't use it.
    // Actually, DefaultLookAndFeel was drawing it relative to 0,0 which is the component top-left.
    // That's fine if the knob is at 0,0 locally.
    
    // 6. Value
    char valStr[16];
    snprintf(valStr, 16, "%.2f", knob.getValue());
    g.drawText(valStr, (bounds.w - AudioUtils::calculateTextWidth(valStr, 9))/2, knobAreaH + 2, 9, 0.6f, 0.6f, 0.6f, 1.0f);
}

void DefaultLookAndFeel::drawLuminousMeter(QuadBatcher& g, LuminousMeter& meter, float level, float peak) {
    auto bounds = meter.getBounds();
    // Reccessed Gutter
    g.drawBeveledRect(0, 0, bounds.w, bounds.h, 2.0f, 0.5f, 0.02f, 0.02f, 0.03f, 1.0f);
    
    if (meter.getOrientation() == LuminousMeter::Orientation::Vertical) {
        int segments = 48; // Higher resolution
        float padding = 1.0f;
        float segHeight = (bounds.h - 4 - (segments - 1) * padding) / segments;
        
        // Convert linear level to dB for display
        float db = (level > 0.00001f) ? 20.0f * std::log10(level) : -100.0f;
        float peakDb = (peak > 0.00001f) ? 20.0f * std::log10(peak) : -100.0f;
        
        // Map dB to 0..1 (Range: -60dB to +6dB)
        float minDb = -60.0f;
        float maxDb = 6.0f;
        auto mapDb = [&](float d) { return std::clamp((d - minDb) / (maxDb - minDb), 0.0f, 1.0f); };
        
        int activeSegs = (int)(mapDb(db) * segments);
        int peakSeg = (int)(mapDb(peakDb) * segments);
        if (peakSeg >= segments) peakSeg = segments - 1;

        for (int i = 0; i < segments; ++i) {
            float y = bounds.h - 2 - (i + 1) * (segHeight + padding) + padding;
            bool active = i < activeSegs;
            bool isPeak = (i == peakSeg);
            
            // Color Scale
            float t = (float)i / segments;
            Color col = Theme::LEDGreen;
            if (t > 0.9f) col = Theme::LEDRed;        // Clip (>0dB approx)
            else if (t > 0.75f) col = Theme::LEDYellow; // Warning (>-12dB approx)

            float intensity = active ? 1.0f : 0.15f;
            if (isPeak) intensity = 1.0f; 

            // LED Body
            g.drawRoundedRect(3, y, bounds.w - 6, segHeight, 1.0f, 0.5f, 
                              col.r * intensity, col.g * intensity, col.b * intensity, 1.0f);
            
            // Subtle Glow for active
            if (active || isPeak) {
                g.drawRoundedRect(2, y, bounds.w - 4, segHeight, 1.0f, 1.0f, 
                                  col.r, col.g, col.b, 0.4f);
            }
        }
    }
}

void DefaultLookAndFeel::drawVUMeter(QuadBatcher& g, VUMeter& meter, float level) {
    auto b = meter.getBounds();
    
    // 1. Shadow
    g.drawQuad(2, 4, b.w, b.h, 0.0f, 0.0f, 0.0f, 0.4f);

    // 2. Bezel (Brushed Aluminum)
    g.drawRoundedGradientRect(0, 0, b.w, b.h, 6.0f, 0.5f, 
                             Theme::Aluminum.r, Theme::Aluminum.g, Theme::Aluminum.b, 1.0f, 
                             Theme::Aluminum.darker(0.4f).r, Theme::Aluminum.darker(0.4f).g, Theme::Aluminum.darker(0.4f).b, 1.0f);
    
    // 3. Glass / Face (Creamy Hardware Paper)
    Rect face = {6, 6, b.w - 12, b.h - 12};
    g.drawRoundedGradientRect(face.x, face.y, face.w, face.h, 4.0f, 0.5f, 
                             0.98f, 0.96f, 0.88f, 1.0f,  // Warm Ivory
                             0.90f, 0.88f, 0.75f, 1.0f); 

    float cx = face.x + face.w * 0.5f;
    float pivotY = face.y + face.h * 1.6f; 
    float needleLen = face.h * 1.4f;
    
    // 4. Scale Markings
    float startAng = -0.6f;
    float endAng = 0.6f;
    for (float t = 0; t <= 1.0f; t += 0.05f) {
        float a = startAng + t * (endAng - startAng);
        float r1 = needleLen - 12.0f;
        float r2 = needleLen - 4.0f;
        float x1 = cx + std::sin(a) * r1;
        float y1 = pivotY - std::cos(a) * r1;
        float x2 = cx + std::sin(a) * r2;
        float y2 = pivotY - std::cos(a) * r2;
        
        bool isRed = (t > 0.8f);
        g.drawLine(x1, y1, x2, y2, (t == 0.0f || t == 0.5f || t == 1.0f) ? 2.5f : 1.0f, 
                   isRed ? Theme::Red.r : 0.1f, isRed ? Theme::Red.g : 0.1f, isRed ? Theme::Red.b : 0.1f, 0.9f);
    }

    // 5. Engraved Branding (Flux Logo)
    g.drawText("BEAM FLUX", cx - 25, face.y + face.h * 0.65f, 10, 0.1f, 0.1f, 0.1f, 0.4f);

    // 6. Needle (Red Hardware Needle)
    float vuAngle = startAng + (std::clamp(level, 0.0f, 1.2f) / 1.2f) * (endAng - startAng);
    float nx = cx + std::sin(vuAngle) * needleLen;
    float ny = pivotY - std::cos(vuAngle) * needleLen;
    
    // Needle Shadow
    g.drawLine(cx + 3, pivotY, nx + 3, ny, 2.0f, 0.0f, 0.0f, 0.0f, 0.2f);
    // Needle
    g.drawLine(cx, pivotY, nx, ny, 2.0f, Theme::Red.r, Theme::Red.g, Theme::Red.b, 1.0f);

    // 7. Pivot Cap
    float capSize = 10.0f;
    g.drawRoundedRect(cx - capSize*0.5f, pivotY - capSize*0.5f, capSize, capSize, capSize*0.5f, 0.5f, 0.1f, 0.1f, 0.1f, 1.0f);
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

    // 1. Shadow
    g.drawQuad(4, 6, bounds.w, bounds.h, 0.0f, 0.0f, 0.0f, 0.4f);

    // 2. Chassis
    g.drawBeveledRect(0, 0, bounds.w, bounds.h, 4.0f, 0.5f, 
                      Theme::Console.r, Theme::Console.g, Theme::Console.b, 1.0f);
    
    // 3. Side Panels
    float panelW = 6.0f;
    g.drawRoundedGradientRect(0, 0, panelW, bounds.h, 2.0f, 0.5f,
                             Theme::Aluminum.r, Theme::Aluminum.g, Theme::Aluminum.b, 1.0f,
                             Theme::Aluminum.darker(0.3f).r, Theme::Aluminum.darker(0.3f).g, Theme::Aluminum.darker(0.3f).b, 1.0f);
    g.drawRoundedGradientRect(bounds.w - panelW, 0, panelW, bounds.h, 2.0f, 0.5f,
                             Theme::Aluminum.r, Theme::Aluminum.g, Theme::Aluminum.b, 1.0f,
                             Theme::Aluminum.darker(0.3f).r, Theme::Aluminum.darker(0.3f).g, Theme::Aluminum.darker(0.3f).b, 1.0f);

    // 4. Header
    float headerH = 24.0f;
    g.drawRoundedGradientRect(panelW, 4, bounds.w - 2*panelW, headerH, 2.0f, 0.5f,
                             Theme::Aluminum.darker(0.1f).r, Theme::Aluminum.darker(0.1f).g, Theme::Aluminum.darker(0.1f).b, 1.0f,
                             Theme::Aluminum.brighter(0.1f).r, Theme::Aluminum.brighter(0.1f).g, Theme::Aluminum.brighter(0.1f).b, 1.0f);

    g.drawText(name, panelW + 10, 10, 11, 1.0f, 1.0f, 1.0f, 0.3f); 
    g.drawText(name, panelW + 9, 9, 11, 0.1f, 0.1f, 0.12f, 0.9f);  
    
    // 5. Close Button
    if (!isMaster) {
        float bx = bounds.w - panelW - 20;
        float by = 8;
        g.drawBeveledRect(bx, by, 14, 14, 2.0f, 0.5f, Theme::LEDRed.r, Theme::LEDRed.g, Theme::LEDRed.b, 1.0f);
        g.drawText("x", bx + 4, by + 1, 10, 1.0f, 1.0f, 1.0f, 0.8f);
    }
    
    // 6. Main Workspace Area
    g.drawRoundedRect(panelW + 2, 32, bounds.w - 2*panelW - 4, bounds.h - 38, 2.0f, 1.0f, 
                      Theme::Black.r, Theme::Black.g, Theme::Black.b, 0.4f);
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

    g.drawRoundedRect(0, 0, bounds.w, bounds.h, 4.0f, 0.5f, r, gr, b, 1.0f);
    
    // Subtle border
    if (isMouseOver && !toggled)
        g.drawRoundedRect(0, 0, bounds.w, bounds.h, 4.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.1f);
}

void ModernLookAndFeel::drawSliderBackground(QuadBatcher& g, Slider& slider, 
                                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle) {
    auto bounds = slider.getBounds();
    // Slot Track
    if (slider.getSliderStyle() == SliderStyle::LinearVertical) {
        float trackW = 4.0f;
        float trackX = (bounds.w - trackW) / 2.0f;
        g.drawRoundedRect(trackX, 0, trackW, bounds.h, 2.0f, 0.5f, 0.05f, 0.05f, 0.07f, 1.0f);
    } else {
        g.drawRoundedRect(0, 0, bounds.w, bounds.h, 2.0f, 0.5f, 0.05f, 0.05f, 0.07f, 1.0f);
    }
}

void ModernLookAndFeel::drawSliderPointer(QuadBatcher& g, Slider& slider, 
                                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle) {
    auto bounds = slider.getBounds();
    
    if (slider.getSliderStyle() == SliderStyle::LinearHorizontal) {
        float w = bounds.w * sliderPos;
        g.drawRoundedRect(0, 0, w, bounds.h, 2.0f, 0.5f, 0.0f, 0.8f, 1.0f, 1.0f); // Keep horizontal modern
    } else {
        // Classic Fader Cap for Vertical
        float h = bounds.h * sliderPos;
        float capH = 28.0f; // Tall fader cap
        float capW = 18.0f; // Wide fader cap
        float capY = bounds.h - h - (capH/2);
        capY = std::clamp(capY, 0.0f, bounds.h - capH);
        float capX = (bounds.w - capW) / 2.0f;

        // Shadow
        g.drawQuad(capX + 2, capY + 4, capW, capH, 0.0f, 0.0f, 0.0f, 0.4f);

        // Body (Dark Grey/Black)
        g.drawBeveledRect(capX, capY, capW, capH, 3.0f, 0.5f, 0.15f, 0.15f, 0.15f, 1.0f);
        
        // Indent / Grip lines
        float lineY = capY + capH * 0.5f;
        g.drawQuad(capX + 2, lineY - 1, capW - 4, 2, 0.8f, 0.8f, 0.8f, 0.9f); // White line
    }
}

void ModernLookAndFeel::drawKnob(QuadBatcher& g, Knob& knob, float sliderPos) {
    auto bounds = knob.getBounds();
    float w = bounds.w;
    float h = bounds.h;
    
    // Center knob in top area
    float knobAreaH = 50.0f; // Match kh from RackUnitUI
    float cx = w * 0.5f;
    float cy = knobAreaH * 0.5f;
    float radius = 20.0f;

    // Rotation angle (CW from TOP)
    float angle = -135.0f + sliderPos * 270.0f; 
    float radAngle = (angle - 90.0f) * 3.14159f / 180.0f;

    auto style = knob.getStyle();
    
    // Base Shadow
    g.drawRoundedRect(cx - radius + 1, cy - radius + 2, radius*2, radius*2, radius, 
                      0.5f, 0.0f, 0.0f, 0.0f, 0.3f);

    if (style == Theme::KnobStyle::ClassicBakelite || style == Theme::KnobStyle::FlutedIndustrial) {
        // === BAKELITE STYLE ===
        // Large, fluted or rounded body
        g.drawRoundedRect(cx - radius, cy - radius, radius*2, radius*2, radius, 
                          0.5f, Theme::Bakelite.r, Theme::Bakelite.g, Theme::Bakelite.b, 1.0f);
        
        // Grip Fluting (Industrial look)
        if (style == Theme::KnobStyle::FlutedIndustrial) {
            for (float a = 0; a < 6.28f; a += 0.4f) {
                float fx = cx + std::cos(a) * (radius * 0.9f);
                float fy = cy + std::sin(a) * (radius * 0.9f);
                g.drawRoundedRect(fx - 1.5f, fy - 1.5f, 3, 3, 1.5f, 0.5f, 1.0f, 1.0f, 1.0f, 0.1f);
            }
        }

        // Center Chrome Cap
        float capRadius = radius * (style == Theme::KnobStyle::FlutedIndustrial ? 0.6f : 0.4f);
        g.drawRoundedGradientRect(cx - capRadius, cy - capRadius, capRadius*2, capRadius*2, capRadius, 
                                  0.5f, 0.6f, 0.62f, 0.65f, 1.0f, 0.3f, 0.32f, 0.35f, 1.0f);
        
        // White engraved needle
        float notchLen = radius * 0.95f;
        float ix = cx + std::cos(radAngle) * notchLen;
        float iy = cy + std::sin(radAngle) * notchLen;
        float ix2 = cx + std::cos(radAngle) * (radius * 0.3f);
        float iy2 = cy + std::sin(radAngle) * (radius * 0.3f);
        g.drawLine(ix2, iy2, ix, iy, 2.5f, 0.95f, 0.95f, 0.95f, 1.0f);
    } 
    else if (style == Theme::KnobStyle::ModernColored) {
        // === COLORED CAP STYLE (SSL/maag) ===
        // Plastic body
        g.drawRoundedRect(cx - radius, cy - radius, radius*2, radius*2, radius, 
                          0.5f, 0.15f, 0.15f, 0.16f, 1.0f);
        
        // Colored Top Cap
        float capRadius = radius * 0.85f;
        Color col = Theme::Emerald; // Default
        g.drawRoundedGradientRect(cx - capRadius, cy - capRadius, capRadius*2, capRadius*2, capRadius, 
                                  0.5f, col.r, col.g, col.b, 1.0f, col.darker(0.3f).r, col.darker(0.3f).g, col.darker(0.3f).b, 1.0f);
        
        // White pointer dot
        float dotR = radius * 0.75f;
        float px = cx + std::cos(radAngle) * dotR;
        float py = cy + std::sin(radAngle) * dotR;
        g.drawRoundedRect(px - 2, py - 2, 4, 4, 2.0f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f);
    }
    else if (style == Theme::KnobStyle::BrushedAluminum) {
        // === ALUMINUM STYLE ===
        g.drawRoundedGradientRect(cx - radius, cy - radius, radius*2, radius*2, radius, 
                                  0.5f, 0.8f, 0.82f, 0.85f, 1.0f, 0.5f, 0.52f, 0.55f, 1.0f);
        
        // Circular brush marks (Inner rings)
        for (float r = 0.2f; r < 0.9f; r += 0.2f) {
            float ringR = radius * r;
            g.drawRoundedRect(cx - ringR, cy - ringR, ringR*2, ringR*2, ringR, 0.5f, 1.0f, 1.0f, 1.0f, 0.05f);
        }

        // Cyan accent indicator
        float notchLen = radius * 0.9f;
        float ix = cx + std::cos(radAngle) * notchLen;
        float iy = cy + std::sin(radAngle) * notchLen;
        g.drawLine(cx, cy, ix, iy, 2.0f, 0.2f, 0.9f, 1.0f, 1.0f); 
    }

    // Value Arc (subtle glow - adjusted for style)
    if (style != Theme::KnobStyle::ClassicBakelite) {
        float arcRadius = radius + 4;
        Color arcCol = (style == Theme::KnobStyle::BrushedAluminum) ? Color(0.2f, 0.9f, 1.0f, 0.5f) : Theme::Emerald.withAlpha(0.5f);
        for (float a = -135.0f; a < angle; a += 10.0f) {
            float rA = (a - 90.0f) * 3.14159f / 180.0f;
            float ax = cx + std::cos(rA) * arcRadius;
            float ay = cy + std::sin(rA) * arcRadius;
            g.drawRoundedRect(ax - 1, ay - 1, 2, 2, 1.0f, 0.5f, arcCol.r, arcCol.g, arcCol.b, 0.4f);
        }
    }
    
    // === LABEL is now handled by Knob's TextElement child component ===
}

void ModernLookAndFeel::drawLuminousMeter(QuadBatcher& g, LuminousMeter& meter, float level, float peak) {
    auto b = meter.getBounds();
    bool isHorizontal = (meter.getOrientation() == LuminousMeter::Orientation::Horizontal);
    
    // Background panel (inset look)
    g.drawRoundedRect(0, 0, b.w, b.h, 3.0f, 0.5f, 0.02f, 0.02f, 0.03f, 1.0f);
    g.drawRoundedRect(1, 1, b.w - 2, b.h - 2, 2.0f, 0.5f, 0.05f, 0.05f, 0.06f, 1.0f);
    
    // LED Segments
    int numSegments = isHorizontal ? 20 : 12;
    float segmentGap = 1.5f;
    
    float mainSize = isHorizontal ? b.w : b.h;
    float crossSize = isHorizontal ? b.h : b.w;
    float segmentSize = (mainSize - 4 - (numSegments - 1) * segmentGap) / numSegments;
    
    for (int i = 0; i < numSegments; ++i) {
        float segPos = (float)i / (float)(numSegments - 1);
        float segStart = 2 + i * (segmentSize + segmentGap);
        
        // Color: Green (0-0.6) -> Yellow (0.6-0.8) -> Red (0.8-1.0)
        float r, gr, bl;
        if (segPos < 0.6f) {
            r = 0.1f; gr = 0.8f; bl = 0.2f;  // Green
        } else if (segPos < 0.8f) {
            r = 0.9f; gr = 0.8f; bl = 0.1f;  // Yellow
        } else {
            r = 1.0f; gr = 0.2f; bl = 0.1f;  // Red
        }
        
        bool lit = level >= segPos;
        bool isPeak = (peak >= segPos && peak < segPos + (1.0f / numSegments));
        
        float alpha = lit ? 0.95f : 0.15f;
        if (isPeak) alpha = 1.0f;
        
        if (isHorizontal) {
            g.drawRoundedRect(segStart, 2, segmentSize, b.h - 4, 1.5f, 0.5f, r, gr, bl, alpha);
        } else {
            // Vertical: draw from bottom up
            float yPos = b.h - 2 - segStart - segmentSize;
            g.drawRoundedRect(2, yPos, b.w - 4, segmentSize, 1.5f, 0.5f, r, gr, bl, alpha);
        }
    }
    
    // Glow overlay for lit segments (subtle)
    if (level > 0.01f) {
        float fillSize = mainSize * std::clamp(level, 0.0f, 1.0f);
        if (isHorizontal) {
            g.drawRoundedRect(2, 0, fillSize - 4, b.h, 2.0f, 0.5f, 1.0f, 1.0f, 1.0f, 0.05f);
        } else {
            g.drawRoundedRect(0, b.h - fillSize, b.w, fillSize - 4, 2.0f, 0.5f, 1.0f, 1.0f, 1.0f, 0.05f);
        }
    }
}

void ModernLookAndFeel::drawVUMeter(QuadBatcher& g, VUMeter& m, float l) { DefaultLookAndFeel::drawVUMeter(g, m, l); }
void ModernLookAndFeel::drawSpectrumAnalyzer(QuadBatcher& g, SpectrumModule& s) { DefaultLookAndFeel::drawSpectrumAnalyzer(g, s); }
void ModernLookAndFeel::drawAudioModule(QuadBatcher& g, AudioModule& module) {
    auto b = module.getBounds();
    // Modern Dark Card
    g.drawRoundedRect(0, 0, b.w, b.h, 6.0f, 1.0f, 0.08f, 0.08f, 0.09f, 1.0f);
    // Header (Match 30px height from AudioModule)
    g.drawRoundedRect(0, 0, b.w, 30, 6.0f, 0.5f, 0.15f, 0.15f, 0.18f, 1.0f);
    g.drawText(module.getName(), 10, 9, 11, 0.95f, 0.95f, 1.0f, 1.0f);
    
    // Glossy overlay for header
    g.drawRoundedRect(0, 0, b.w, 15, 6.0f, 0.5f, 1.0f, 1.0f, 1.0f, 0.03f);
    
    // Border highlight
    g.drawRoundedRect(0, 0, b.w, b.h, 6.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.08f);

    // Sidechain Label
    if (module.getSidechainPort()) {
        float cx = b.w * 0.5f;
        g.drawText("SC", cx - 6, b.h - 20, 9, 0.5f, 0.5f, 0.5f, 0.8f);
    }
}


} // namespace Beam