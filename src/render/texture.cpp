#include "texture.hpp"
#include <SDL3/SDL.h>
#include <iostream>

// OpenGL constant literals to avoid header conflicts/missing symbols
#define GL_RGBA 0x1908
#define GL_RGB 0x1907
#define GL_UNSIGNED_BYTE 0x1401
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_LINEAR 0x2601
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_TEXTURE0 0x84C0

// External GL function declarations (simple version)
typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
extern "C" {
    __declspec(dllimport) void __stdcall glGenTextures(GLsizei n, GLuint *textures);
    __declspec(dllimport) void __stdcall glBindTexture(GLenum target, GLuint texture);
    __declspec(dllimport) void __stdcall glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
    __declspec(dllimport) void __stdcall glTexParameteri(GLenum target, GLenum pname, GLint param);
    __declspec(dllimport) void __stdcall glDeleteTextures(GLsizei n, const GLuint *textures);
}

// Since glActiveTexture is usually in an extension header (like glad), we use our internal glad if it is working.
// Actually, I'll just include glad.h at the very top and hope for the best.
#include "../../third_party/glad.h"

namespace Beam {

Texture::Texture(const std::string& path) : m_path(path) {
    if (!path.empty() && path != "procedural_metal") {
        load(path);
    }
}

Texture::~Texture() {
    if (m_id) glDeleteTextures(1, &m_id);
}

bool Texture::load(const std::string& path) {
    SDL_Surface* surface = SDL_LoadBMP(path.c_str());
    if (!surface) return false;

    m_width = surface->w;
    m_height = surface->h;

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    GLenum format = 0x1907; // GL_RGB
    if (SDL_BITSPERPIXEL(surface->format) == 32) format = 0x1908; // GL_RGBA

    glTexImage2D(GL_TEXTURE_2D, 0, format, m_width, m_height, 0, format, 0x1401, surface->pixels);
    
    glTexParameteri(GL_TEXTURE_2D, 0x2802, 0x812F);
    glTexParameteri(GL_TEXTURE_2D, 0x2803, 0x812F);
    glTexParameteri(GL_TEXTURE_2D, 0x2801, 0x2601);
    glTexParameteri(GL_TEXTURE_2D, 0x2800, 0x2601);

    SDL_DestroySurface(surface);
    return true;
}

void Texture::createRGB(int width, int height, const unsigned char* data) {
    if (m_id) glDeleteTextures(1, &m_id);
    m_width = width;
    m_height = height;
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexImage2D(GL_TEXTURE_2D, 0, 0x1907, width, height, 0, 0x1907, 0x1401, data);
    glTexParameteri(GL_TEXTURE_2D, 0x2802, 0x812F);
    glTexParameteri(GL_TEXTURE_2D, 0x2803, 0x812F);
    glTexParameteri(GL_TEXTURE_2D, 0x2801, 0x2601);
    glTexParameteri(GL_TEXTURE_2D, 0x2800, 0x2601);
}

void Texture::bind(unsigned int slot) const {
    if (glActiveTexture) {
        glActiveTexture(0x84C0 + slot); 
    }
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace Beam
