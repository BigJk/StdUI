#pragma once

#include <string>

namespace Log {

/**
 * @brief Emit a log message via the stdout protocol.
 *
 * Sends {"action":"log","data":{"namespace":"...","message":"..."}} as a
 * single-line JSON message to stdout so the controlling application can
 * handle it.
 *
 * @param nameSpace The namespace/category for the log message.
 * @param pattern   Printf-style format string.
 * @param ...       Variable arguments matching the format string.
 */
void Print(const std::string& nameSpace, const char* pattern, ...);

}  // namespace Log
