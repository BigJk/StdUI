#pragma once

#include <hjson.h>

#include <optional>
#include <string>

namespace IO {

/**
 * @brief Selects the transport mechanism used for reading and writing messages.
 */
enum class TransportMode {
  /// Read from stdin / write to stdout (default).
  StdIO,
  /// Unix domain socket (also supported on Windows 10 1803+).
  UnixSocket,
  /// Named pipe (Unix domain socket on non-Windows; Windows named pipe on Windows).
  NamedPipe,
};

/**
 * @brief Configuration passed to Init() to select and configure the transport.
 */
struct TransportConfig {
  /// The transport to use.
  TransportMode mode = TransportMode::StdIO;

  /// Path for the socket file (UnixSocket), the socket file used as a named-pipe
  /// alias on non-Windows (NamedPipe), or the Windows named-pipe path (NamedPipe on Windows).
  /// Ignored when mode == StdIO.
  std::string path;
};

/**
 * @brief Start the background reader thread for the configured transport.
 *
 * For StdIO the thread blocks on stdin.
 * For UnixSocket the socket is created, bound, and a single client connection
 * is accepted before the reader thread starts.
 * For NamedPipe on Windows a named pipe is created and one client connection
 * is awaited. On all other platforms NamedPipe behaves identically to
 * UnixSocket (a Unix domain socket is used instead of a FIFO, since FIFOs
 * cannot carry bidirectional IPC reliably).
 *
 * Must be called once before any call to ReadLine().
 *
 * @param config Transport configuration. Defaults to StdIO.
 */
void Init(TransportConfig config = {});

/**
 * @brief Stop the background reader thread and release transport resources.
 *
 * Signals the reader thread to stop and joins it.
 * Should be called during application shutdown.
 */
void Shutdown();

/**
 * @brief Try to dequeue a line that was received from the transport.
 *
 * Non-blocking. Returns std::nullopt if no complete line is available yet.
 *
 * @return The next line, or std::nullopt if none is available.
 */
std::optional<std::string> ReadLine();

/**
 * @brief Block until a line is available from the transport, then return it.
 *
 * Polls ReadLine() every 10ms until a line is available.
 *
 * @return The next line.
 */
std::string MustReadLine();

/**
 * @brief Try to read a line and parse it as an Hjson value.
 *
 * Non-blocking. Returns std::nullopt if no line is available.
 *
 * @return Parsed Hjson::Value, or std::nullopt if no line is available.
 */
std::optional<Hjson::Value> ReadValue();

/**
 * @brief Block until a line is available and parse it as an Hjson value.
 *
 * Polls every 10ms until a line is available, then parses and returns it.
 *
 * @return Parsed Hjson::Value.
 */
Hjson::Value MustReadValue();

/**
 * @brief Write a string to the transport.
 *
 * Does not append a newline. Flushes immediately.
 *
 * @param s The string to write.
 */
void Write(const std::string &s);

/**
 * @brief Write a string to the transport followed by a newline.
 *
 * Flushes immediately.
 *
 * @param s The string to write.
 */
void WriteLine(const std::string &s);

/**
 * @brief Serialize an Hjson::Value to the transport followed by a newline.
 *
 * Flushes immediately.
 *
 * @param value The Hjson value to serialize and write.
 */
void WriteValue(const Hjson::Value &value);

}  // namespace IO
