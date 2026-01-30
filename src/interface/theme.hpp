#ifndef THEME_HPP
#define THEME_HPP

namespace Beam {

struct Colour {
    float r, g, b, a;
    
    Colour(float red, float green, float blue, float alpha = 1.0f) 
        : r(red), g(green), b(blue), a(alpha) {}
        
    Colour withAlpha(float newAlpha) const { return Colour(r, g, b, newAlpha); }
    
    // Helper to darken/lighten
    Colour brighter(float amount = 0.1f) const {
        return Colour(std::min(r + amount, 1.0f), 
                      std::min(g + amount, 1.0f), 
                      std::min(b + amount, 1.0f), a);
    }
    
    Colour darker(float amount = 0.1f) const {
        return Colour(std::max(r - amount, 0.0f), 
                      std::max(g - amount, 0.0f), 
                      std::max(b - amount, 0.0f), a);
    }
};

struct Theme {
    // Brand Colors
    static constexpr Colour black   = {0.05f, 0.05f, 0.06f, 1.0f};
    static constexpr Colour white   = {0.95f, 0.95f, 0.95f, 1.0f};
    static constexpr Colour emerald = {0.13f, 0.62f, 0.42f, 1.0f};
    static constexpr Colour red     = {0.80f, 0.10f, 0.10f, 1.0f};
    static constexpr Colour grey    = {0.15f, 0.15f, 0.16f, 1.0f};
    
    // Component Colors
    Colour background = black;
    Colour panel      = grey;
    Colour text       = white;
    Colour accent     = emerald;
    Colour warning    = red;
    
    // Metrics
    float cornerRadius = 4.0f;
    float padding = 4.0f;
};

} // namespace Beam

#endif
