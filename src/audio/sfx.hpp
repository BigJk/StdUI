#pragma once

#include <string>

/**
 * @namespace SFX
 * @brief Sound effect system
 */
namespace SFX {
/**
 * @brief Plays a sound effect.
 * @param sfx Absolute or relative path to the sound file.
 */
void Play(const std::string& sfx);

/**
 * @brief Mutes or unmutes a sound effect.
 * @param sfx Path to the sound file.
 * @param mute True to mute, false to unmute
 */
void SetMute(const std::string& sfx, bool mute);

/**
 * @brief Checks if a sound effect is muted.
 * @param sfx Path to the sound file.
 * @return True if the sound effect is muted, false otherwise
 */
bool IsMuted(const std::string& sfx);

/**
 * @brief Starts looping a sound effect.
 * @param sfx Path to the sound file.
 */
void StartLoop(const std::string& sfx);

/**
 * @brief Stops looping a sound effect.
 * @param sfx Path to the sound file.
 */
void StopLoop(const std::string& sfx);

/**
 * @brief Stops all instances of a sound effect.
 * @param sfx Path to the sound file.
 */
void Stop(const std::string& sfx);

/**
 * @brief Gets the length of a sound effect in seconds.
 * @param sfx Path to the sound file.
 * @return The length of the sound in seconds, or 0.0 if not found
 */
double Length(const std::string& sfx);

/**
 * @brief Updates the sound effect system.
 * Should be called once per frame.
 */
void Update();

/**
 * @brief Cleans up resources used by the sound effect system.
 * Should be called before the program exits.
 */
void Cleanup();
}  // namespace SFX
