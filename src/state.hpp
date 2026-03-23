#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "layout.hpp"
#include "raylib.h"

/**
 * @brief Shared element state — keyed by element id, holds current value as a string.
 * Updated by set-value actions and by interactive elements themselves.
 */
extern std::unordered_map<std::string, std::string> ELEMENT_STATE;

/**
 * @brief Per-pane HTML content, keyed by pane id.
 * Updated by update-content actions. The default pane id is "main".
 */
extern std::unordered_map<std::string, std::string> PANE_CONTENT;

/**
 * @brief The active layout tree. Defaults to a single pane with id "main".
 * Replaced by set-pane-layout actions.
 */
extern std::shared_ptr<Layout::Node> LAYOUT;

/**
 * @brief Set to true when a close action is received from the controlling process.
 * The main loop checks this flag and exits cleanly.
 */
extern std::atomic<bool> CLOSE_REQUESTED;

/**
 * @brief Holds the data for a pending confirm dialog, if any.
 * Set by the confirm action handler; cleared after the user responds.
 */
struct ConfirmDialog {
  std::string id;
  std::string question;
  std::string title;
  std::string okText;
  std::string cancelText;
};

/**
 * @brief The id of the pane currently being rendered.
 * Set by RenderNode before each ImHTML::Canvas call so that element event
 * handlers can tag outbound events with the correct pane id.
 */
extern std::string CURRENT_PANE;

extern std::optional<ConfirmDialog> CONFIRM_PENDING;

/**
 * @brief Pending scroll requests per pane id.
 *
 * Each entry holds a target Y value and a frame countdown.  The countdown
 * starts at 1 so the scroll is deferred by one frame: on the same frame that
 * update-content arrives the document height is not yet known, so we wait one
 * additional frame for litehtml to lay out the new content before applying
 * SetScrollY.
 *
 * - target < 0  → "scroll to bottom" (resolved to GetScrollMaxY() at apply time)
 * - target >= 0 → absolute pixel offset from the top
 *
 * Entries are erased once they have been applied (countdown == 0).
 */
struct ScrollRequest {
  float target;    ///< Desired scroll position (-1.0f = bottom).
  int framesLeft;  ///< Frames remaining before the scroll is applied.
};
extern std::unordered_map<std::string, ScrollRequest> SCROLL_REQUESTS;

/**
 * @brief A single toast notification.
 *
 * Toasts are created by the `toast` action and rendered as decoration-free
 * ImGui windows stacked from the top-center of the display. Each toast has
 * an HTML content string, explicit pixel dimensions, a time-to-live in
 * seconds, and the time at which it was created (used to compute remaining
 * life). A dismissed flag allows early removal via double-click.
 */
struct Toast {
  std::string content;  ///< HTML to render inside the toast.
  float width;          ///< Width of the toast window in pixels.
  float height;         ///< Height of the toast window in pixels.
  float ttl;            ///< Total time-to-live in seconds.
  double createdAt;     ///< GetTime() value at creation.
  bool dismissed;       ///< True once the user double-clicks to dismiss early.
};

/**
 * @brief The ordered list of active toasts.
 *
 * Toasts are appended to the back (newest at the bottom of the stack) and
 * removed from the front once expired or dismissed.
 */
extern std::vector<Toast> TOASTS;

/**
 * @brief A registered keyboard shortcut.
 *
 * Each keybind has an application-defined id that is echoed back in the
 * key-pressed event, plus a set of modifier flags and the target key.
 */
struct Keybind {
  std::string id;   ///< Application-defined identifier echoed in key-pressed.
  KeyboardKey key;  ///< The primary key (raylib KeyboardKey enum value).
  bool ctrl;        ///< Require Ctrl (left or right) to be held.
  bool shift;       ///< Require Shift (left or right) to be held.
  bool alt;         ///< Require Alt (left or right) to be held.
  bool meta;        ///< Require Super/Cmd (left or right) to be held.
};

/**
 * @brief Registered keybinds, keyed by application-defined id.
 * Set by the set-keybinds action. Checked every frame in the renderer.
 */
extern std::unordered_map<std::string, Keybind> KEYBINDS;
