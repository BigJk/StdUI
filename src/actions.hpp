#pragma once

#include <string>

/**
 * @namespace Actions
 * @brief Inbound action handling and outbound event emission.
 *
 * Actions::Init() registers all handlers for messages arriving from the
 * controlling process.  Call Actions::Process() once per frame to dispatch
 * the next pending message (if any).
 */
namespace Actions {

/**
 * @brief Registers all inbound action handlers.
 * Must be called once before the main loop starts.
 */
void Init();

/**
 * @brief Reads and dispatches one pending action from the IO queue.
 *
 * Returns true if an action was dequeued and dispatched, false if the queue
 * was empty. Call in a loop each frame to drain all pending actions:
 *
 * @code
 *   while (Actions::Process()) {}
 * @endcode
 *
 * @return true if an action was processed, false if the queue was empty.
 */
bool Process();

/**
 * @brief Emits an input-changed event for the given element id and value.
 * Also updates ELEMENT_STATE so the next render reflects the new value.
 *
 * @param id    The element id.
 * @param value The new value as a string.
 */
void EmitInputChanged(const std::string &id, const std::string &value);

}  // namespace Actions
