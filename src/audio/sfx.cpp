#include "sfx.hpp"

#include <filesystem>
#include <map>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "log.hpp"
#include "raylib.h"
#include "settings.hpp"

namespace SFX {
namespace {
struct SoundInstance {
  Sound sound;
  bool inUse;
};
std::unordered_map<std::string, std::vector<SoundInstance>> soundPools;
std::unordered_set<std::string> mutedSounds;
std::unordered_map<std::string, SoundInstance *> loopingSounds;
std::unordered_set<std::string> playedThisFrame;
std::map<std::string, double> soundLengthCache;
const int MAX_SOUND_INSTANCES = 16;
}  // namespace

void Play(const std::string &sfx) {
  if (IsMuted(sfx)) {
    return;
  }

  auto [_, inserted] = playedThisFrame.insert(sfx);
  if (!inserted) {
    return;
  }

  // Load sound if not in pool
  if (soundPools.find(sfx) == soundPools.end()) {
    Sound baseSound = LoadSound((sfx).c_str());
    soundPools[sfx] = std::vector<SoundInstance>();
    soundPools[sfx].push_back({baseSound, false});
  }

  auto &pool = soundPools[sfx];

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
    SetSoundVolume(freeInstance->sound, Settings::Get()->sfxVolume);
    PlaySound(freeInstance->sound);
    freeInstance->inUse = true;
  }
}

void PlayPitchVariance(const std::string &sfx, float variance) {
  if (IsMuted(sfx)) {
    return;
  }

  auto [_, inserted] = playedThisFrame.insert(sfx);
  if (!inserted) {
    return;
  }

  if (soundPools.find(sfx) == soundPools.end()) {
    Sound baseSound = LoadSound(sfx.c_str());
    soundPools[sfx] = std::vector<SoundInstance>();
    soundPools[sfx].push_back({baseSound, false});
  }

  auto &pool = soundPools[sfx];

  SoundInstance *freeInstance = nullptr;
  for (auto &instance : pool) {
    if (!instance.inUse && !IsSoundPlaying(instance.sound)) {
      freeInstance = &instance;
      break;
    }
  }

  if (!freeInstance && pool.size() < MAX_SOUND_INSTANCES) {
    Sound newSound = LoadSoundAlias(pool[0].sound);
    pool.push_back({newSound, false});
    freeInstance = &pool.back();
  }

  if (freeInstance) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(1.0f - variance, 1.0f + variance);

    SetSoundVolume(freeInstance->sound, Settings::Get()->sfxVolume);
    SetSoundPitch(freeInstance->sound, dis(gen));
    PlaySound(freeInstance->sound);
    freeInstance->inUse = true;
  }
}

void SetMute(const std::string &sfx, bool mute) {
  if (mute) {
    mutedSounds.insert(sfx);
  } else {
    mutedSounds.erase(sfx);
  }
}

bool IsMuted(const std::string &sfx) { return mutedSounds.find(sfx) != mutedSounds.end(); }

void StartLoop(const std::string &sfx) {
  if (loopingSounds.find(sfx) != loopingSounds.end()) {
    return;
  }

  if (IsMuted(sfx)) {
    return;
  }

  // Load sound if not in pool
  if (soundPools.find(sfx) == soundPools.end()) {
    Sound baseSound = LoadSound(sfx.c_str());
    soundPools[sfx] = std::vector<SoundInstance>();
    soundPools[sfx].push_back({baseSound, false});
  }

  auto &pool = soundPools[sfx];

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
    SetSoundVolume(freeInstance->sound, Settings::Get()->sfxVolume);
    PlaySound(freeInstance->sound);
    freeInstance->inUse = true;
    loopingSounds[sfx] = freeInstance;
  }
}

void StopLoop(const std::string &sfx) {
  auto it = loopingSounds.find(sfx);
  if (it != loopingSounds.end()) {
    StopSound(it->second->sound);
    it->second->inUse = false;
    loopingSounds.erase(it);
  }
}

void Stop(const std::string &sfx) {
  auto it = soundPools.find(sfx);
  if (it != soundPools.end()) {
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
  auto cacheIt = soundLengthCache.find(sfx);
  if (cacheIt != soundLengthCache.end()) {
    return cacheIt->second;
  }

  // Load sound if not in pool
  if (soundPools.find(sfx) == soundPools.end()) {
    Sound baseSound = LoadSound(sfx.c_str());
    soundPools[sfx] = std::vector<SoundInstance>();
    soundPools[sfx].push_back({baseSound, false});
  }

  auto &pool = soundPools[sfx];
  // Calculate length from frameCount (standard sample rate is 44100 Hz)
  double length = (double)pool[0].sound.frameCount / 44100.0;
  soundLengthCache[sfx] = length;
  return length;
}

void Update() {
  for (auto &[name, pool] : soundPools) {
    for (auto &instance : pool) {
      if (instance.inUse && !IsSoundPlaying(instance.sound)) {
        instance.inUse = false;
      }
    }
  }

  // Restart looping sounds that have finished
  for (auto it = loopingSounds.begin(); it != loopingSounds.end();) {
    if (!IsSoundPlaying(it->second->sound)) {
      SetSoundVolume(it->second->sound, Settings::Get()->sfxVolume);
      PlaySound(it->second->sound);
    }
    ++it;
  }

  playedThisFrame.clear();
}

void Cleanup() {
  // Sound cache cleanup
  for (auto &[name, pool] : soundPools) {
    // Only the first is the original, the rest are aliases
    UnloadSound(pool[0].sound);
  }
  soundPools.clear();
}
}  // namespace SFX
