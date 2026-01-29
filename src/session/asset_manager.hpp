#ifndef ASSET_MANAGER_HPP
#define ASSET_MANAGER_HPP

#include "../render/texture.hpp"
#include <map>
#include <string>
#include <memory>
#include <mutex>

namespace Beam {

/**
 * @class AssetManager
 * @brief Centralized repository for sharing heavy resources like Textures.
 * 
 * The AssetManager ensures that any given file is loaded into memory/GPU only once.
 * It uses a reference-counted approach (std::shared_ptr) to manage asset lifecycles.
 */
class AssetManager {
public:
    static AssetManager& instance() {
        static AssetManager inst;
        return inst;
    }

    /**
     * @brief Retrieves a texture from the cache or loads it if not present.
     * @param path The filesystem path to the texture file.
     * @return A shared pointer to the Texture object, or nullptr if loading fails.
     */
    std::shared_ptr<Texture> getTexture(const std::string& path);

    /**
     * @brief Removes unused assets from the cache.
     */
    void purgeUnused();

    /**
     * @brief Clears the entire asset cache.
     */
    void clear();

private:
    AssetManager() = default; // Private constructor for singleton
    std::map<std::string, std::shared_ptr<Texture>> m_textures;
    std::mutex m_mutex;
};

} // namespace Beam

#endif // ASSET_MANAGER_HPP






