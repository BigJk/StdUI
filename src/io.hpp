#pragma once

#include <hjson.h>

#include <optional>
#include <string>

namespace IO {

/**
 * @brief Start the background stdin reader thread.
 *
 * Must be called once before any call to ReadLine().
 * The thread blocks on stdin and enqueues complete lines.
 */
void Init();

/**
 * @brief Stop the background stdin reader thread.
 *
 * Signals the reader thread to stop and joins it.
 * Should be called during application shutdown.
 */
void Shutdown();

/**
 * @brief Try to dequeue a line that was received from stdin.
 *
 * Non-blocking. Returns std::nullopt if no complete line is available yet.
 *
 * @return The next line from stdin, or std::nullopt if none is available.
 */
std::optional<std::string> ReadLine();

/**
 * @brief Block until a line is available from stdin, then return it.
 *
 * Polls ReadLine() every 10ms until a line is available.
 *
 * @return The next line from stdin.
 */
std::string MustReadLine();

/**
 * @brief Try to read a line from stdin and parse it as an Hjson value.
 *
 * Non-blocking. Returns std::nullopt if no line is available.
 *
 * @return Parsed Hjson::Value, or std::nullopt if no line is available.
 */
std::optional<Hjson::Value> ReadValue();

/**
 * @brief Block until a line is available from stdin and parse it as an Hjson value.
 *
 * Polls every 10ms until a line is available, then parses and returns it.
 *
 * @return Parsed Hjson::Value.
 */
Hjson::Value MustReadValue();

/**
 * @brief Write a string to stdout.
 *
 * Does not append a newline. Flushes immediately.
 *
 * @param s The string to write.
 */
void Write(const std::string &s);

/**
 * @brief Write a string to stdout followed by a newline.
 *
 * Flushes immediately.
 *
 * @param s The string to write.
 */
void WriteLine(const std::string &s);

/**
 * @brief Serialize an Hjson::Value to stdout followed by a newline.
 *
 * Flushes immediately.
 *
 * @param value The Hjson value to serialize and write.
 */
void WriteValue(const Hjson::Value &value);

}  // namespace IO
