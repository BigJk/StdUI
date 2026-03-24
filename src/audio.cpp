#include "audio.hpp"

#include <filesystem>
#include <map>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "log.hpp"
#include "raylib.h"
#include "settings.hpp"

namespace Audio {
namespace {
struct SoundInstance {
  Sound sound;
  bool inUse;
};
std::unordered_map<std::string, std::vector<SoundInstance>> SOUND_POOLS;
std::unordered_map<std::string, SoundInstance *> LOOPING_SOUNDS;
std::unordered_set<std::string> PLAYED_THIS_FRAME;
std::map<std::string, double> SOUND_LENGTH_CACHE;
const int MAX_SOUND_INSTANCES = 16;
}  // namespace

void Play(const std::string &sfx) {
  auto [_, inserted] = PLAYED_THIS_FRAME.insert(sfx);
  if (!inserted) {
    return;
  }

  // Load sound if not in pool
  if (SOUND_POOLS.find(sfx) == SOUND_POOLS.end()) {
    Sound baseSound = LoadSound((sfx).c_str());
    SOUND_POOLS[sfx] = std::vector<SoundInstance>();
    SOUND_POOLS[sfx].push_back({baseSound, false});
  }

  auto &pool = SOUND_POOLS[sfx];

  // Find free instance or create new one
  SoundInstance *freeInstance = nullptr;
  for (auto &instance : pool) {
    if (!instance.inUse && !IsSoundPlaying(instance.sound)) {
      freeInstance = &instance;
      break;
    }
  }

  // If no free instance found and pool not at max size, create new instance
  if (!freeInstance && pool.size() < MAX_SOUND_INSTANCES) {
    Sound newSound = LoadSoundAlias(pool[0].sound);
    pool.push_back({newSound, false});
    freeInstance = &pool.back();
  }

  // If we found or created a free instance, play it
  if (freeInstance) {
    SetSoundVolume(freeInstance->sound, Settings::Get()->audioVolume);
    PlaySound(freeInstance->sound);
    freeInstance->inUse = true;
  }
}

void Stop(const std::string &sfx) {
  auto it = SOUND_POOLS.find(sfx);
  if (it != SOUND_POOLS.end()) {
    for (auto &instance : it->second) {
      if (IsSoundPlaying(instance.sound)) {
        StopSound(instance.sound);
        instance.inUse = false;
      }
    }
  }
}

double Length(const std::string &sfx) {
  // Check cache first
  auto cacheIt = SOUND_LENGTH_CACHE.find(sfx);
  if (cacheIt != SOUND_LENGTH_CACHE.end()) {
    return cacheIt->second;
  }

  // Load sound if not in pool
  if (SOUND_POOLS.find(sfx) == SOUND_POOLS.end()) {
    Sound baseSound = LoadSound(sfx.c_str());
    SOUND_POOLS[sfx] = std::vector<SoundInstance>();
    SOUND_POOLS[sfx].push_back({baseSound, false});
  }

  auto &pool = SOUND_POOLS[sfx];
  // Calculate length from frameCount (standard sample rate is 44100 Hz)
  double length = (double)pool[0].sound.frameCount / 44100.0;
  SOUND_LENGTH_CACHE[sfx] = length;
  return length;
}

void Update() {
  for (auto &[name, pool] : SOUND_POOLS) {
    for (auto &instance : pool) {
      if (instance.inUse && !IsSoundPlaying(instance.sound)) {
        instance.inUse = false;
      }
    }
  }

  // Restart looping sounds that have finished
  for (auto it = LOOPING_SOUNDS.begin(); it != LOOPING_SOUNDS.end();) {
    if (!IsSoundPlaying(it->second->sound)) {
      SetSoundVolume(it->second->sound, Settings::Get()->audioVolume);
      PlaySound(it->second->sound);
    }
    ++it;
  }

  PLAYED_THIS_FRAME.clear();
}

void Cleanup() {
  // Sound cache cleanup
  for (auto &[name, pool] : SOUND_POOLS) {
    // Only the first is the original, the rest are aliases
    UnloadSound(pool[0].sound);
  }
  SOUND_POOLS.clear();
}
}  // namespace Audio
