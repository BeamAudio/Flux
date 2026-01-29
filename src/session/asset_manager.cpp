#include "asset_manager.hpp"
#include <iostream>

namespace Beam {

std::shared_ptr<Texture> AssetManager::getTexture(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_textures.find(path);
    if (it != m_textures.end()) {
        return it->second;
    }

    // Not in cache, try to load
    auto texture = std::make_shared<Texture>(path);
    if (texture->getID() != 0) {
        m_textures[path] = texture;
        return texture;
    }

    return nullptr;
}

void AssetManager::purgeUnused() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_textures.begin(); it != m_textures.end(); ) {
        if (it->second.use_count() == 1) { // Only held by the manager
            it = m_textures.erase(it);
        } else {
            ++it;
        }
    }
}

void AssetManager::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_textures.clear();
}

} // namespace Beam






