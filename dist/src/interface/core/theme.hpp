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
    static constexpr Color Black   = {0.05f, 0.05f, 0.06f, 1.0f};
    static constexpr Color White   = {0.95f, 0.95f, 0.95f, 1.0f};
    static constexpr Color Emerald = {0.13f, 0.62f, 0.42f, 1.0f};
    static constexpr Color Red     = {0.56f, 0.03f, 0.03f, 1.0f};
    static constexpr Color Grey    = {0.15f, 0.15f, 0.16f, 1.0f};
    static constexpr Color GreyDark = {0.10f, 0.10f, 0.11f, 1.0f};
    static constexpr Color GreyLight = {0.25f, 0.25f, 0.26f, 1.0f};

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

    static constexpr Color Bakelite = {0.07f, 0.07f, 0.08f, 1.0f};
    static constexpr Color Console  = {0.12f, 0.14f, 0.16f, 1.0f}; // Navy-Grey
    static constexpr Color Aluminum = {0.70f, 0.72f, 0.75f, 1.0f};
    static constexpr Color Wood     = {0.35f, 0.20f, 0.12f, 1.0f};
    static constexpr Color LEDGreen = {0.20f, 0.90f, 0.30f, 1.0f};
    static constexpr Color LEDRed   = {1.00f, 0.10f, 0.10f, 1.0f};
    static constexpr Color LEDYellow= {0.90f, 0.85f, 0.10f, 1.0f};
};

} // namespace Beam

#endif // THEME_HPP