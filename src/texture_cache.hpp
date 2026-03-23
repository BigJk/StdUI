#pragma once

#include <string>

#include "raylib.h"

namespace TextureCache {
/**
 * @brief Retrieves a texture from the cache or loads it if not present.
 * @param path The file path of the texture.
 * @return The loaded Texture2D.
 */
Texture2D Get(const std::string& path);

/**
 * @brief Retrieves a pointer to a texture from the cache or loads it if not present.
 * @param path The file path of the texture.
 * @return Pointer to the loaded Texture2D.
 */
Texture2D* GetPtr(const std::string& path);

/**
 * @brief Adds a texture to the cache.
 * @param path The file path to associate with the texture.
 * @param texture The Texture2D to add to the cache.
 * @return The added Texture2D.
 */
Texture2D Add(const std::string& path, Texture2D texture);
}  // namespace TextureCache
