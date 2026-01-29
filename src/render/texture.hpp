#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <string>
#include "../../third_party/glad.h"

namespace Beam {

/**
 * @class Texture
 * @brief Represents an OpenGL texture resource.
 */
class Texture {
public:
    /**
     * @brief Constructs a texture from a file path.
     * @param path Path to the image file (BMP supported).
     */
    Texture(const std::string& path);
    ~Texture();

    /**
     * @brief Loads a texture from disk.
     * @param path Path to the BMP file.
     * @return true if successful.
     */
    bool load(const std::string& path);

    /**
     * @brief Creates a texture from raw RGB data in memory.
     * @param width Width in pixels.
     * @param height Height in pixels.
     * @param data Raw byte array.
     */
    void createRGB(int width, int height, const unsigned char* data);

    /**
     * @brief Binds the texture to a specific OpenGL slot.
     * @param slot Texture unit (0-15).
     */
    void bind(unsigned int slot = 0) const;
    void unbind() const;

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    unsigned int getID() const { return m_id; }

private:
    unsigned int m_id = 0;
    int m_width = 0;
    int m_height = 0;
    std::string m_path;
};

} // namespace Beam

#endif // TEXTURE_HPP





