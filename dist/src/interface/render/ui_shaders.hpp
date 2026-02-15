#ifndef UI_SHADERS_HPP
#define UI_SHADERS_HPP

namespace Beam {

const char* UI_VERTEX_SHADER = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;

out vec2 TexCoord;
out vec4 Color;

uniform mat4 projection;
uniform float uPanX;
uniform float uPanY;
uniform float uOriginX;
uniform float uOriginY;
uniform float uZoom;

void main() {
    TexCoord = aTexCoord;
    Color = aColor;
    vec2 pos = aPos;
    // Global Coordinate Equation:
    // 1. Scale relative to Virtual Origin (0,0)
    // 2. Apply Pan (also in Screen Scale, but represents Virtual movement)
    // 3. Add Screen Origin (Host Window Position)
    pos = (pos * uZoom) + vec2(uPanX, uPanY) + vec2(uOriginX, uOriginY);
    gl_Position = projection * vec4(pos, 0.0, 1.0);
}
)";

const char* UI_FRAGMENT_SHADER = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 Color;

uniform sampler2D uiTexture;
uniform int mode; 

uniform float uSizeX;
uniform float uSizeY;
uniform float uRadius;
uniform float uEdgeSoftness;

float roundedRectSDF(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

float hash(vec2 p) {
    p = fract(p*vec2(123.34, 456.21));
    p += dot(p, p+45.32);
    return fract(p.x*p.y);
}

void main() {
    if (mode == 1) {
        vec4 texColor = texture(uiTexture, TexCoord);
        FragColor = vec4(Color.rgb, Color.a * texColor.r);
    } 
    else if (mode == 2) {
        vec2 uSize = vec2(uSizeX, uSizeY);
        vec2 center = uSize * 0.5;
        vec2 p = TexCoord * uSize - center;
        float dist = roundedRectSDF(p, center, uRadius);
        float alpha = 1.0 - smoothstep(-uEdgeSoftness, uEdgeSoftness, dist);
        FragColor = vec4(Color.rgb, Color.a * alpha);
    }
    else if (mode == 4) { // Beveled Rounded Rect
        vec2 uSize = vec2(uSizeX, uSizeY);
        vec2 center = uSize * 0.5;
        vec2 p = TexCoord * uSize - center;
        float dist = roundedRectSDF(p, center, uRadius);
        
        float alpha = 1.0 - smoothstep(-uEdgeSoftness, uEdgeSoftness, dist);
        
        // Pseudo-lighting - Extremely subtle
        float light = dot(normalize(vec2(1.0, -1.0)), normalize(TexCoord - 0.5));
        float bevelWidth = 2.0;
        float edge = smoothstep(-bevelWidth, 0.0, dist);
        
        vec3 finalColor = Color.rgb + (light * 0.02 * edge); // Reduced to 0.02
        FragColor = vec4(finalColor, Color.a * alpha);
    }
    else if (mode == 5) { // Chassis Panel (Enhanced Hardware Materials)
        vec2 uSize = vec2(uSizeX, uSizeY);
        vec2 center = uSize * 0.5;
        vec2 p = TexCoord * uSize - center;
        float dist = roundedRectSDF(p, center, uRadius);
        float alpha = 1.0 - smoothstep(-uEdgeSoftness, uEdgeSoftness, dist);
        
        float grain = hash(gl_FragCoord.xy);
        float noise = (grain - 0.5) * 0.03; 
        
        // 1. Brushed Aluminum Texture (Anisotropic)
        float brush = hash(vec2(floor(TexCoord.y * uSizeY * 2.0), 0.0)) * 0.04; 
        
        // 2. Light / Bevel (Top-down)
        float light = dot(normalize(vec2(0.0, -1.0)), normalize(TexCoord - 0.5));
        float edge = smoothstep(-3.0, 0.0, dist);

        // 3. Global Polish: Scanlines & Vignette
        float scanline = sin(gl_FragCoord.y * 1.5) * 0.02;
        float vignette = 1.0 - length(TexCoord - 0.5) * 0.3;
        
        vec3 finalColor = Color.rgb;
        
        if (Color.r > 0.4 && Color.g > 0.4 && Color.b > 0.4) { // Aluminum/Steel profile
            finalColor += noise;
            finalColor += brush;
            finalColor += (light * 0.1 * edge);
        } else { // Dark paint/Bakelite profile
            finalColor *= (1.0 + noise * 3.0); 
            float border = smoothstep(0.0, 1.5, abs(dist + 1.5));
            finalColor *= (0.5 + 0.5 * border);
        }

        finalColor += scanline;
        finalColor *= vignette;

        FragColor = vec4(finalColor, Color.a * alpha);
    }
    else {
        FragColor = Color;
    }
}
)";

} // namespace Beam

#endif