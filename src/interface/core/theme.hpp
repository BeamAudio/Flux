#ifndef THEME_HPP
#define THEME_HPP

#include <algorithm>

namespace Beam {

/**
 * @struct Color
 * @brief Simple RGBA color representation with helpers.
 */
struct Color {
    float r, g, b, a;
    
    constexpr Color() : r(0), g(0), b(0), a(1) {}
    constexpr Color(float red, float green, float blue, float alpha = 1.0f) 
        : r(red), g(green), b(blue), a(alpha) {}
        
    Color withAlpha(float newAlpha) const { return Color(r, g, b, newAlpha); }
    
    Color brighter(float amount = 0.1f) const {
        return Color((std::min)(r + amount, 1.0f), 
                     (std::min)(g + amount, 1.0f), 
                     (std::min)(b + amount, 1.0f), a);
    }
    
    Color darker(float amount = 0.1f) const {
        return Color((std::max)(r - amount, 0.0f), 
                     (std::max)(g - amount, 0.0f), 
                     (std::max)(b - amount, 0.0f), a);
    }
};

/**
 * @class Theme
 * @brief Centralized brand identity and color palette.
 */
class Theme {
public:
    // Brand Colors
    static constexpr Color Black   = {0.002f, 0.002f, 0.002f, 1.0f}; // Near Black
    static constexpr Color White   = {0.95f, 0.95f, 0.95f, 1.0f};
    static constexpr Color Emerald = {0.02f, 0.95f, 0.40f, 1.0f}; // Very Intense
    static constexpr Color Red     = {0.95f, 0.01f, 0.01f, 1.0f}; // Very Intense
    static constexpr Color Grey    = {0.005f, 0.005f, 0.006f, 1.0f}; // Much Darker
    static constexpr Color GreyDark = {0.002f, 0.002f, 0.003f, 1.0f}; // Near Black
    static constexpr Color GreyLight = {0.012f, 0.012f, 0.014f, 1.0f}; // Dark Grey
    
    // UI Accents
    static constexpr Color Primary   = {0.0f, 0.48f, 1.0f, 1.0f}; // Beam Blue
    static constexpr Color Secondary = {0.2f, 0.22f, 0.25f, 1.0f}; // Control Surface Grey
    static constexpr Color Accent    = {1.0f, 0.65f, 0.0f, 1.0f}; // Orange

    // --- Design Library Tokens ---
    
    enum class MaterialType {
        Standard,
        BrushedAluminum,
        Bakelite,
        WrinklePaint
    };

    enum class KnobStyle {
        ClassicBakelite,
        ModernColored,
        BrushedAluminum,
        FlutedIndustrial
    };

    static constexpr Color Bakelite = {0.005f, 0.005f, 0.006f, 1.0f}; // Deep Deep Black
    static constexpr Color Console  = {0.008f, 0.008f, 0.01f, 1.0f}; // Even Darker
    static constexpr Color Aluminum = {0.50f, 0.52f, 0.55f, 1.0f}; 
    static constexpr Color Wood     = {0.15f, 0.08f, 0.04f, 1.0f}; 
    static constexpr Color LEDGreen = {0.00f, 1.00f, 0.05f, 1.0f}; 
    static constexpr Color LEDRed   = {1.00f, 0.00f, 0.00f, 1.0f}; 
    static constexpr Color LEDYellow= {1.00f, 1.00f, 0.00f, 1.0f}; 
};

} // namespace Beam

#endif // THEME_HPP