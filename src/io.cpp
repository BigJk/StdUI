#include "io.hpp"

#include <hjson.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>

namespace IO {

namespace {

std::thread THREAD;
std::mutex MUTEX;
std::queue<std::string> QUEUE;
std::atomic<bool> RUNNING{false};

void ReaderThread() {
  std::string line;
  while (RUNNING.load()) {
    if (!std::getline(std::cin, line)) {
      // EOF or error — stop the thread
      RUNNING.store(false);
      break;
    }
    std::lock_guard<std::mutex> lock(MUTEX);
    QUEUE.push(std::move(line));
  }
}

}  // namespace

void Init() {
  RUNNING.store(true);
  THREAD = std::thread(ReaderThread);
}

void Shutdown() {
  RUNNING.store(false);
  // Closing stdin unblocks getline so the thread can exit cleanly.
  std::cin.setstate(std::ios::eofbit);
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
  std::cout << s;
  std::cout.flush();
}

void WriteLine(const std::string &s) {
  std::cout << s << '\n';
  std::cout.flush();
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
  std::cout << Hjson::Marshal(value, opt) << '\n';
  std::cout.flush();
}

}  // namespace IO
