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

void main() {
    // Basic color rendering for now
    FragColor = Color;
}
)";

} // namespace Beam

#endif // UI_SHADERS_HPP
