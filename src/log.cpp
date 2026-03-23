#include "log.hpp"

#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <string>

#include "hjson.h"
#include "io.hpp"

namespace Log {

namespace {
std::mutex LOG_MUTEX;
}  // namespace

void Print(const std::string& nameSpace, const char* pattern, ...) {
  // Format the message
  char buffer[2048];
  va_list args;
  va_start(args, pattern);
  vsnprintf(buffer, sizeof(buffer), pattern, args);
  va_end(args);

  std::lock_guard<std::mutex> lock(LOG_MUTEX);

  Hjson::Value data;
  data["namespace"] = nameSpace;
  data["message"] = std::string(buffer);

  Hjson::Value msg;
  msg["action"] = "log";
  msg["data"] = data;

  IO::WriteValue(msg);
}

}  // namespace Log
