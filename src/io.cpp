#include "io.hpp"

#include <hjson.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>

// Platform-specific socket / pipe headers
#ifndef _WIN32
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#else
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace IO {

namespace {

// ---------------------------------------------------------------------------
// Shared state
// ---------------------------------------------------------------------------

std::thread THREAD;
std::mutex MUTEX;
std::queue<std::string> QUEUE;
std::atomic<bool> RUNNING{false};

TransportConfig CONFIG;

// Write mutex so concurrent WriteValue calls don't interleave bytes.
std::mutex WRITE_MUTEX;

// ---------------------------------------------------------------------------
// Platform-specific transport state
// ---------------------------------------------------------------------------

#ifndef _WIN32
// Unix domain socket (and --pipe alias on Unix): the accepted connection fd, or -1.
int SOCKET_FD{-1};
#else
// Windows named pipe handle.
HANDLE PIPE_HANDLE{INVALID_HANDLE_VALUE};
#endif

// ---------------------------------------------------------------------------
// Low-level write helpers
// ---------------------------------------------------------------------------

/**
 * @brief Write all bytes of @p data to the active transport.
 *
 * @param data Pointer to the byte buffer.
 * @param len  Number of bytes to write.
 */
void RawWrite(const char *data, std::size_t len) {
  switch (CONFIG.mode) {
    case TransportMode::StdIO:
      std::cout.write(data, static_cast<std::streamsize>(len));
      std::cout.flush();
      break;

#ifndef _WIN32
    case TransportMode::UnixSocket: {
      std::size_t written = 0;
      while (written < len) {
        ssize_t n = ::send(SOCKET_FD, data + written, len - written, 0);
        if (n <= 0) break;
        written += static_cast<std::size_t>(n);
      }
      break;
    }

    case TransportMode::NamedPipe: {
      // On Unix, --pipe is backed by a Unix domain socket (same as --socket).
      std::size_t written = 0;
      while (written < len) {
        ssize_t n = ::send(SOCKET_FD, data + written, len - written, 0);
        if (n <= 0) break;
        written += static_cast<std::size_t>(n);
      }
      break;
    }
#else
    case TransportMode::UnixSocket: {
      // On Windows, UDS is surfaced through the Winsock API the same way.
      std::size_t written = 0;
      while (written < len) {
        int n = ::send(reinterpret_cast<SOCKET>(SOCKET_FD), data + written, static_cast<int>(len - written), 0);
        if (n <= 0) break;
        written += static_cast<std::size_t>(n);
      }
      break;
    }

    case TransportMode::NamedPipe: {
      DWORD total = 0;
      while (total < static_cast<DWORD>(len)) {
        DWORD wrote = 0;
        if (!WriteFile(PIPE_HANDLE, data + total, static_cast<DWORD>(len) - total, &wrote, nullptr)) break;
        total += wrote;
      }
      break;
    }
#endif
  }
}

// ---------------------------------------------------------------------------
// Reader thread implementations
// ---------------------------------------------------------------------------

/**
 * @brief Reader thread body for StdIO transport.
 *
 * Blocks on std::getline(std::cin, …) and pushes complete lines onto QUEUE.
 */
void ReaderThreadStdIO() {
  std::string line;
  while (RUNNING.load()) {
    if (!std::getline(std::cin, line)) {
      RUNNING.store(false);
      break;
    }
    std::lock_guard<std::mutex> lock(MUTEX);
    QUEUE.push(std::move(line));
  }
}

#ifndef _WIN32

/**
 * @brief Reader thread body for Unix domain socket transport.
 *
 * Reads bytes from SOCKET_FD and reassembles newline-delimited lines.
 */
void ReaderThreadSocket() {
  std::string buf;
  char chunk[4096];
  while (RUNNING.load()) {
    ssize_t n = ::recv(SOCKET_FD, chunk, sizeof(chunk), 0);
    if (n <= 0) {
      RUNNING.store(false);
      break;
    }
    buf.append(chunk, static_cast<std::size_t>(n));
    std::size_t pos;
    while ((pos = buf.find('\n')) != std::string::npos) {
      std::string line = buf.substr(0, pos);
      buf.erase(0, pos + 1);
      std::lock_guard<std::mutex> lock(MUTEX);
      QUEUE.push(std::move(line));
    }
  }
}

#else  // _WIN32

/**
 * @brief Reader thread body for Unix domain socket transport on Windows.
 *
 * Windows 10 1803+ exposes UDS through Winsock.
 */
void ReaderThreadSocket() {
  std::string buf;
  char chunk[4096];
  while (RUNNING.load()) {
    int n = ::recv(reinterpret_cast<SOCKET>(SOCKET_FD), chunk, sizeof(chunk), 0);
    if (n <= 0) {
      RUNNING.store(false);
      break;
    }
    buf.append(chunk, static_cast<std::size_t>(n));
    std::size_t pos;
    while ((pos = buf.find('\n')) != std::string::npos) {
      std::string line = buf.substr(0, pos);
      buf.erase(0, pos + 1);
      std::lock_guard<std::mutex> lock(MUTEX);
      QUEUE.push(std::move(line));
    }
  }
}

/**
 * @brief Reader thread body for Windows named-pipe transport.
 */
void ReaderThreadNamedPipe() {
  std::string buf;
  char chunk[4096];
  while (RUNNING.load()) {
    DWORD bytesRead = 0;
    BOOL ok = ReadFile(PIPE_HANDLE, chunk, sizeof(chunk), &bytesRead, nullptr);
    if (!ok || bytesRead == 0) {
      RUNNING.store(false);
      break;
    }
    buf.append(chunk, static_cast<std::size_t>(bytesRead));
    std::size_t pos;
    while ((pos = buf.find('\n')) != std::string::npos) {
      std::string line = buf.substr(0, pos);
      buf.erase(0, pos + 1);
      std::lock_guard<std::mutex> lock(MUTEX);
      QUEUE.push(std::move(line));
    }
  }
}

#endif  // _WIN32

// ---------------------------------------------------------------------------
// Transport setup helpers
// ---------------------------------------------------------------------------

#ifndef _WIN32

/**
 * @brief Create and bind a Unix domain socket, then accept one connection.
 *
 * @param path Filesystem path for the socket file.
 * @throws std::runtime_error on any system-call failure.
 */
void SetupUnixSocket(const std::string &path) {
  // Remove a stale socket file if present.
  ::unlink(path.c_str());

  int server_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
  if (server_fd < 0) {
    throw std::runtime_error("IO: socket() failed: " + std::string(::strerror(errno)));
  }

  struct sockaddr_un addr {};
  addr.sun_family = AF_UNIX;
  if (path.size() >= sizeof(addr.sun_path)) {
    ::close(server_fd);
    throw std::runtime_error("IO: socket path too long");
  }
  std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

  if (::bind(server_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) {
    ::close(server_fd);
    throw std::runtime_error("IO: bind() failed: " + std::string(::strerror(errno)));
  }

  if (::listen(server_fd, 1) < 0) {
    ::close(server_fd);
    throw std::runtime_error("IO: listen() failed: " + std::string(::strerror(errno)));
  }

  // Block until the controlling process connects.
  SOCKET_FD = ::accept(server_fd, nullptr, nullptr);
  ::close(server_fd);  // We only need one connection.

  if (SOCKET_FD < 0) {
    throw std::runtime_error("IO: accept() failed: " + std::string(::strerror(errno)));
  }
}

#else  // _WIN32

/**
 * @brief Create and bind a Unix domain socket on Windows (Winsock), then
 *        accept one connection.
 *
 * @param path Filesystem path for the socket file.
 * @throws std::runtime_error on any system-call failure.
 */
void SetupUnixSocket(const std::string &path) {
  WSADATA wsa{};
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    throw std::runtime_error("IO: WSAStartup failed");
  }

  SOCKET server_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
  if (server_fd == INVALID_SOCKET) {
    throw std::runtime_error("IO: socket() failed");
  }

  struct sockaddr_un addr {};
  addr.sun_family = AF_UNIX;
  if (path.size() >= sizeof(addr.sun_path)) {
    ::closesocket(server_fd);
    throw std::runtime_error("IO: socket path too long");
  }
  std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

  // Remove stale socket file.
  DeleteFileA(path.c_str());

  if (::bind(server_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == SOCKET_ERROR) {
    ::closesocket(server_fd);
    throw std::runtime_error("IO: bind() failed");
  }

  if (::listen(server_fd, 1) == SOCKET_ERROR) {
    ::closesocket(server_fd);
    throw std::runtime_error("IO: listen() failed");
  }

  SOCKET client = ::accept(server_fd, nullptr, nullptr);
  ::closesocket(server_fd);

  if (client == INVALID_SOCKET) {
    throw std::runtime_error("IO: accept() failed");
  }

  // Store as int for uniformity with the Unix path.
  SOCKET_FD = static_cast<int>(client);
}

/**
 * @brief Create a Windows named pipe at @p path and wait for one connection.
 *
 * @p path should be in the form \\.\pipe\<name>.
 *
 * @param path Named-pipe path.
 * @throws std::runtime_error on any system-call failure.
 */
void SetupNamedPipe(const std::string &path) {
  PIPE_HANDLE = CreateNamedPipeA(path.c_str(),
                                 PIPE_ACCESS_DUPLEX,
                                 PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                                 1,       // max instances
                                 65536,   // out buffer
                                 65536,   // in buffer
                                 0,       // default timeout
                                 nullptr  // default security
  );

  if (PIPE_HANDLE == INVALID_HANDLE_VALUE) {
    throw std::runtime_error("IO: CreateNamedPipe() failed");
  }

  // Block until the controlling process connects.
  if (!ConnectNamedPipe(PIPE_HANDLE, nullptr)) {
    DWORD err = GetLastError();
    if (err != ERROR_PIPE_CONNECTED) {
      CloseHandle(PIPE_HANDLE);
      PIPE_HANDLE = INVALID_HANDLE_VALUE;
      throw std::runtime_error("IO: ConnectNamedPipe() failed");
    }
  }
}

#endif  // _WIN32

}  // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void Init(TransportConfig config) {
  CONFIG = config;
  RUNNING.store(true);

  switch (config.mode) {
    case TransportMode::StdIO:
      THREAD = std::thread(ReaderThreadStdIO);
      break;

    case TransportMode::UnixSocket:
      SetupUnixSocket(config.path);
      THREAD = std::thread(ReaderThreadSocket);
      break;

    case TransportMode::NamedPipe:
#ifndef _WIN32
      // On Unix, named pipes (FIFOs) cannot carry bidirectional IPC reliably.
      // We implement --pipe as a Unix domain socket so behaviour is identical
      // to --socket on all non-Windows platforms.
      SetupUnixSocket(config.path);
      THREAD = std::thread(ReaderThreadSocket);
#else
      SetupNamedPipe(config.path);
      THREAD = std::thread(ReaderThreadNamedPipe);
#endif
      break;
  }
}

void Shutdown() {
  RUNNING.store(false);

  switch (CONFIG.mode) {
    case TransportMode::StdIO:
      std::cin.setstate(std::ios::eofbit);
      break;

#ifndef _WIN32
    case TransportMode::UnixSocket:
    case TransportMode::NamedPipe:
      // NamedPipe on Unix is backed by a Unix domain socket (see Init).
      if (SOCKET_FD >= 0) {
        ::shutdown(SOCKET_FD, SHUT_RDWR);
        ::close(SOCKET_FD);
        SOCKET_FD = -1;
        ::unlink(CONFIG.path.c_str());
      }
      break;
#else
    case TransportMode::UnixSocket:
      if (SOCKET_FD >= 0) {
        ::closesocket(static_cast<SOCKET>(SOCKET_FD));
        SOCKET_FD = -1;
        DeleteFileA(CONFIG.path.c_str());
        WSACleanup();
      }
      break;

    case TransportMode::NamedPipe:
      if (PIPE_HANDLE != INVALID_HANDLE_VALUE) {
        DisconnectNamedPipe(PIPE_HANDLE);
        CloseHandle(PIPE_HANDLE);
        PIPE_HANDLE = INVALID_HANDLE_VALUE;
      }
      break;
#endif
  }

  if (THREAD.joinable()) {
    THREAD.join();
  }
}

std::optional<std::string> ReadLine() {
  std::lock_guard<std::mutex> lock(MUTEX);
  if (QUEUE.empty()) {
    return std::nullopt;
  }
  std::string line = std::move(QUEUE.front());
  QUEUE.pop();
  return line;
}

std::string MustReadLine() {
  while (true) {
    if (auto line = ReadLine()) {
      return *line;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

std::optional<Hjson::Value> ReadValue() {
  auto line = ReadLine();
  if (!line) {
    return std::nullopt;
  }
  return Hjson::Unmarshal(line->c_str(), line->size());
}

Hjson::Value MustReadValue() {
  while (true) {
    if (auto value = ReadValue()) {
      return *value;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void Write(const std::string &s) {
  std::lock_guard<std::mutex> lock(WRITE_MUTEX);
  RawWrite(s.data(), s.size());
}

void WriteLine(const std::string &s) {
  std::lock_guard<std::mutex> lock(WRITE_MUTEX);
  RawWrite(s.data(), s.size());
  RawWrite("\n", 1);
}

void WriteValue(const Hjson::Value &value) {
  Hjson::EncoderOptions opt;
  opt.bracesSameLine = true;
  opt.quoteAlways = true;
  opt.quoteKeys = true;
  opt.separator = true;
  opt.comments = false;
  opt.indentBy = "";
  opt.eol = "";
  std::string serialized = Hjson::Marshal(value, opt) + "\n";
  std::lock_guard<std::mutex> lock(WRITE_MUTEX);
  RawWrite(serialized.data(), serialized.size());
}

}  // namespace IO
