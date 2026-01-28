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

void main() {
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
    Color = aColor;
}
)";

const char* UI_FRAGMENT_SHADER = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 Color;

uniform sampler2D uiTexture;
uniform int mode; // 0: Solid/Gradient, 1: Textured/Font, 2: Rounded Rect

uniform float uSizeX;
uniform float uSizeY;
uniform float uRadius;
uniform float uEdgeSoftness;

float roundedRectSDF(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

void main() {
    if (mode == 1) {
        // Font / Texture Mode
        vec4 texColor = texture(uiTexture, TexCoord);
        FragColor = vec4(Color.rgb, Color.a * texColor.r);
    } 
    else if (mode == 2) {
        // High-fidelity Rounded Rect with SDF
        vec2 uSize = vec2(uSizeX, uSizeY);
        vec2 center = uSize * 0.5;
        vec2 p = TexCoord * uSize - center;
        float dist = roundedRectSDF(p, center, uRadius);
        
        float alpha = 1.0 - smoothstep(-uEdgeSoftness, uEdgeSoftness, dist);
        FragColor = vec4(Color.rgb, Color.a * alpha);
    }
    else {
        // Basic Mode (supports simple gradients via vertex color)
        FragColor = Color;
    }
}
)";

} // namespace Beam

#endif // UI_SHADERS_HPP
