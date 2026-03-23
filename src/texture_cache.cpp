#include "texture_cache.hpp"

#include <cstring>
#include <unordered_map>
#include <vector>

#include "raylib.h"

namespace TextureCache {
namespace {
std::vector<Texture2D*> TEXTURE_PTR_CACHE = {};
std::unordered_map<std::string, int> TEXTURE_CACHE = {};
}  // namespace

Texture2D Get(const std::string& path) {
  auto it = TEXTURE_CACHE.find(path);
  if (it != TEXTURE_CACHE.end()) {
    return *TEXTURE_PTR_CACHE[it->second];
  }

  Texture2D texture = LoadTexture(path.c_str());
  TEXTURE_PTR_CACHE.push_back(new Texture2D(texture));
  TEXTURE_CACHE.insert_or_assign(path, TEXTURE_PTR_CACHE.size() - 1);
  return *TEXTURE_PTR_CACHE.back();
}

Texture2D* GetPtr(const std::string& path) {
  auto it = TEXTURE_CACHE.find(path);
  if (it != TEXTURE_CACHE.end()) {
    return TEXTURE_PTR_CACHE[it->second];
  }

  Texture2D texture = LoadTexture(path.c_str());
  TEXTURE_PTR_CACHE.push_back(new Texture2D(texture));
  TEXTURE_CACHE.insert_or_assign(path, TEXTURE_PTR_CACHE.size() - 1);
  return TEXTURE_PTR_CACHE.back();
}

Texture2D Add(const std::string& path, Texture2D texture) {
  TEXTURE_PTR_CACHE.push_back(new Texture2D(texture));
  TEXTURE_CACHE.insert_or_assign(path, TEXTURE_PTR_CACHE.size() - 1);
  return texture;
}
}  // namespace TextureCache
