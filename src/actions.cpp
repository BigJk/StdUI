#include "actions.hpp"

#include <cctype>
#include <functional>
#include <string>
#include <unordered_map>

#include "action.hpp"
#include "audio/sfx.hpp"
#include "hjson.h"
#include "io.hpp"
#include "layout.hpp"
#include "log.hpp"
#include "raylib.h"
#include "settings.hpp"
#include "state.hpp"

namespace Actions {

namespace {

/**
 * @brief Converts a lowercase key name string to a raylib KeyboardKey.
 *
 * Accepts single characters (e.g. "a", "1", "/"), named keys
 * (e.g. "enter", "escape", "f1"–"f12", "space", "tab", "backspace",
 * "delete", "insert", "home", "end", "pageup", "pagedown",
 * "left", "right", "up", "down", "capslock"), and returns
 * KEY_NULL for unrecognised strings.
 *
 * @param name Lowercase key name.
 * @return The matching KeyboardKey, or KEY_NULL.
 */
KeyboardKey ParseKeyName(const std::string &name) {
  static const std::unordered_map<std::string, KeyboardKey> TABLE = {
      {"space", KEY_SPACE},
      {"enter", KEY_ENTER},
      {"return", KEY_ENTER},
      {"tab", KEY_TAB},
      {"backspace", KEY_BACKSPACE},
      {"insert", KEY_INSERT},
      {"delete", KEY_DELETE},
      {"del", KEY_DELETE},
      {"right", KEY_RIGHT},
      {"left", KEY_LEFT},
      {"down", KEY_DOWN},
      {"up", KEY_UP},
      {"pageup", KEY_PAGE_UP},
      {"pagedown", KEY_PAGE_DOWN},
      {"home", KEY_HOME},
      {"end", KEY_END},
      {"capslock", KEY_CAPS_LOCK},
      {"scrolllock", KEY_SCROLL_LOCK},
      {"numlock", KEY_NUM_LOCK},
      {"printscreen", KEY_PRINT_SCREEN},
      {"pause", KEY_PAUSE},
      {"escape", KEY_ESCAPE},
      {"esc", KEY_ESCAPE},
      {"f1", KEY_F1},
      {"f2", KEY_F2},
      {"f3", KEY_F3},
      {"f4", KEY_F4},
      {"f5", KEY_F5},
      {"f6", KEY_F6},
      {"f7", KEY_F7},
      {"f8", KEY_F8},
      {"f9", KEY_F9},
      {"f10", KEY_F10},
      {"f11", KEY_F11},
      {"f12", KEY_F12},
      {"'", KEY_APOSTROPHE},
      {",", KEY_COMMA},
      {"-", KEY_MINUS},
      {".", KEY_PERIOD},
      {"/", KEY_SLASH},
      {";", KEY_SEMICOLON},
      {"=", KEY_EQUAL},
      {"[", KEY_LEFT_BRACKET},
      {"\\", KEY_BACKSLASH},
      {"]", KEY_RIGHT_BRACKET},
      {"`", KEY_GRAVE},
  };

  auto it = TABLE.find(name);
  if (it != TABLE.end()) return it->second;

  // Single ASCII digit 0-9
  if (name.size() == 1 && name[0] >= '0' && name[0] <= '9') {
    return static_cast<KeyboardKey>(KEY_ZERO + (name[0] - '0'));
  }
  // Single ASCII letter a-z
  if (name.size() == 1 && name[0] >= 'a' && name[0] <= 'z') {
    return static_cast<KeyboardKey>(KEY_A + (name[0] - 'a'));
  }

  return KEY_NULL;
}

}  // namespace

/**
 * @brief Emits an input-changed event for the given element id and value.
 *
 * @param id    The element id.
 * @param value The new value as a string.
 */
void EmitInputChanged(const std::string &id, const std::string &value) {
  ELEMENT_STATE[id] = value;
  Hjson::Value msg;
  msg["action"] = "input-changed";
  msg["data"]["id"] = id;
  msg["data"]["value"] = value;
  msg["data"]["pane"] = CURRENT_PANE;
  IO::WriteValue(msg);
}

/**
 * @brief Registers all inbound action handlers.
 */
void Init() {
  // Nothing to initialise at runtime — handlers are registered lazily in
  // Process() via a static local map.
}

/**
 * @brief Reads and dispatches one pending action from the IO queue.
 *
 * @return true if an action was dequeued and dispatched, false if the queue
 *         was empty.
 */
bool Process() {
  using Handler = std::function<void(const Hjson::Value &)>;
  static const std::unordered_map<std::string, Handler> HANDLERS = {
      {"update-content",
       [](const Hjson::Value &data) {
         // Backwards-compatible: plain string targets the default "main" pane.
         if (data.type() == Hjson::Type::String) {
           PANE_CONTENT["main"] = data.to_string();
         } else if (data.type() == Hjson::Type::Map) {
           auto pane = data["pane"];
           auto content = data["content"];
           if (pane.defined() && content.defined()) {
             PANE_CONTENT[pane.to_string()] = content.to_string();
           }
         }
       }},

      {"set-value",
       [](const Hjson::Value &data) {
         if (data.type() != Hjson::Type::Map) return;
         auto id = data["id"];
         auto value = data["value"];
         if (id.defined() && value.defined()) {
           ELEMENT_STATE[id.to_string()] = value.to_string();
         }
       }},

      {"set-title",
       [](const Hjson::Value &data) {
         if (data.type() == Hjson::Type::String) {
           SetWindowTitle(data.to_string().c_str());
         }
       }},

      {"set-window-icon",
       [](const Hjson::Value &data) {
         // data: plain string path, or object with a "path" field.
         std::string path;
         if (data.type() == Hjson::Type::String) {
           path = data.to_string();
         } else if (data.type() == Hjson::Type::Map && data["path"].defined()) {
           path = data["path"].to_string();
         }
         if (path.empty()) return;

         Image img = LoadImage(path.c_str());
         if (img.data == nullptr) {
           Log::Print("Main", "set-window-icon: failed to load image: %s", path.c_str());
           return;
         }
         SetWindowIcon(img);
         UnloadImage(img);
       }},

      {"set-window-icons",
       [](const Hjson::Value &data) {
         // data: array of path strings, e.g. ["icon16.png", "icon32.png", "icon64.png"]
         if (data.type() != Hjson::Type::Vector || data.size() == 0) return;

         std::vector<Image> images;
         images.reserve(data.size());
         for (int i = 0; i < static_cast<int>(data.size()); ++i) {
           if (data[i].type() != Hjson::Type::String) continue;
           std::string path = data[i].to_string();
           Image img = LoadImage(path.c_str());
           if (img.data == nullptr) {
             Log::Print("Main", "set-window-icons: failed to load image: %s", path.c_str());
             continue;
           }
           images.push_back(img);
         }
         if (!images.empty()) {
           SetWindowIcons(images.data(), static_cast<int>(images.size()));
         }
         for (auto &img : images) {
           UnloadImage(img);
         }
       }},

      {"get-value",
       [](const Hjson::Value &data) {
         std::string id;
         if (data.type() == Hjson::Type::String) {
           id = data.to_string();
         } else if (data.type() == Hjson::Type::Map && data["id"].defined()) {
           id = data["id"].to_string();
         }
         Hjson::Value msg;
         msg["action"] = "value-result";
         msg["data"]["id"] = id;
         msg["data"]["value"] = ELEMENT_STATE.count(id) ? ELEMENT_STATE[id] : "";
         IO::WriteValue(msg);
       }},

      {"close", [](const Hjson::Value &) { CLOSE_REQUESTED = true; }},
      {"minimize", [](const Hjson::Value &) { MinimizeWindow(); }},
      {"maximize", [](const Hjson::Value &) { MaximizeWindow(); }},

      {"set-fps",
       [](const Hjson::Value &data) {
         int fps = 60;
         if (data.type() == Hjson::Type::Double || data.type() == Hjson::Type::Int64) {
           fps = static_cast<int>(data.to_int64());
         } else if (data.type() == Hjson::Type::Map && data["fps"].defined()) {
           fps = static_cast<int>(data["fps"].to_int64());
         }
         if (fps > 0) {
           SetTargetFPS(fps);
         }
       }},

      {"set-position",
       [](const Hjson::Value &data) {
         if (data.type() != Hjson::Type::Map) return;
         auto x = data["x"];
         auto y = data["y"];
         if (x.defined() && y.defined()) {
           SetWindowPosition(static_cast<int>(x.to_double()), static_cast<int>(y.to_double()));
         }
       }},

      {"play-sound",
       [](const Hjson::Value &data) {
         std::string file;
         if (data.type() == Hjson::Type::String) {
           file = data.to_string();
         } else if (data.type() == Hjson::Type::Map && data["file"].defined()) {
           file = data["file"].to_string();
         }
         if (!file.empty()) {
           SFX::Play(file);
         }
       }},

      {"set-volume",
       [](const Hjson::Value &data) {
         float volume = 1.0f;
         if (data.type() == Hjson::Type::Double || data.type() == Hjson::Type::Int64) {
           volume = static_cast<float>(data.to_double());
         } else if (data.type() == Hjson::Type::Map && data["volume"].defined()) {
           volume = static_cast<float>(data["volume"].to_double());
         } else {
           return;
         }
         if (volume < 0.0f) volume = 0.0f;
         if (volume > 1.0f) volume = 1.0f;
         Settings::Get()->sfxVolume = volume;
       }},

      {"confirm",
       [](const Hjson::Value &data) {
         if (data.type() != Hjson::Type::Map) return;
         if (!data["id"].defined() || !data["question"].defined()) return;

         ConfirmDialog dlg;
         dlg.id = data["id"].to_string();
         dlg.question = data["question"].to_string();
         dlg.title = data["title"].defined() ? data["title"].to_string() : "Confirm";
         dlg.okText = data["ok-text"].defined() ? data["ok-text"].to_string() : "OK";
         dlg.cancelText = data["cancel-text"].defined() ? data["cancel-text"].to_string() : "Cancel";
         CONFIRM_PENDING = dlg;
       }},

      {"set-pane-layout",
       [](const Hjson::Value &data) {
         auto root = Layout::Parse(data);
         if (root) {
           LAYOUT = root;
           // Clear content for all panes so stale HTML from a previous layout
           // does not bleed into newly created panes.
           PANE_CONTENT.clear();
         } else {
           Log::Print("Layout", "Failed to parse set-pane-layout data");
         }
       }},

      {"toast",
       [](const Hjson::Value &data) {
         if (data.type() != Hjson::Type::Map) return;
         if (!data["content"].defined()) return;

         Toast t;
         t.content = data["content"].to_string();
         t.width = data["width"].defined() ? static_cast<float>(data["width"].to_double()) : 300.0f;
         t.height = data["height"].defined() ? static_cast<float>(data["height"].to_double()) : 0.0f;
         t.ttl = data["ttl"].defined() ? static_cast<float>(data["ttl"].to_double()) : 3.0f;
         t.createdAt = GetTime();
         t.dismissed = false;
         TOASTS.push_back(std::move(t));
       }},

      {"set-clipboard-text",
       [](const Hjson::Value &data) {
         std::string text;
         if (data.type() == Hjson::Type::String) {
           text = data.to_string();
         } else if (data.type() == Hjson::Type::Map && data["text"].defined()) {
           text = data["text"].to_string();
         } else {
           return;
         }
         SetClipboardText(text.c_str());
       }},

      {"get-clipboard-text",
       [](const Hjson::Value &) {
         const char *raw = GetClipboardText();
         Hjson::Value msg;
         msg["action"] = "clipboard-text-result";
         msg["data"]["text"] = raw ? std::string(raw) : std::string("");
         IO::WriteValue(msg);
       }},

      {"scroll-to",
       [](const Hjson::Value &data) {
         if (data.type() != Hjson::Type::Map) return;
         auto pane = data["pane"];
         auto pos = data["position"];
         if (!pane.defined()) return;

         std::string paneId = pane.to_string();
         // Defer by 1 frame so litehtml can lay out any new content that
         // arrived in the same batch before we apply SetScrollY.
         float target = 0.0f;
         if (pos.defined() && pos.type() == Hjson::Type::String && pos.to_string() == "bottom") {
           target = -1.0f;
         } else if (pos.defined() && (pos.type() == Hjson::Type::Double || pos.type() == Hjson::Type::Int64)) {
           target = static_cast<float>(pos.to_double());
         }
         SCROLL_REQUESTS[paneId] = ScrollRequest{target, 1};
       }},

      {"set-keybinds",
       [](const Hjson::Value &data) {
         // Accepts an array of keybind objects:
         // [{ "id": "save", "key": "s", "ctrl": true }, ...]
         // Replaces the entire KEYBINDS table each time it is called.
         if (data.type() != Hjson::Type::Vector) return;
         KEYBINDS.clear();
         for (int i = 0; i < static_cast<int>(data.size()); ++i) {
           const Hjson::Value &entry = data[i];
           if (entry.type() != Hjson::Type::Map) continue;
           if (!entry["id"].defined() || !entry["key"].defined()) continue;

           std::string keyName = entry["key"].to_string();
           // Normalise to lowercase so users can write "S" or "s" interchangeably.
           for (char &c : keyName) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

           KeyboardKey kk = ParseKeyName(keyName);
           if (kk == KEY_NULL) {
             Log::Print("Keybind", "Unrecognised key name: %s", keyName.c_str());
             continue;
           }

           Keybind kb;
           kb.id = entry["id"].to_string();
           kb.key = kk;
           kb.ctrl = entry["ctrl"].defined() && entry["ctrl"].to_int64() != 0;
           kb.shift = entry["shift"].defined() && entry["shift"].to_int64() != 0;
           kb.alt = entry["alt"].defined() && entry["alt"].to_int64() != 0;
           kb.meta = entry["meta"].defined() && entry["meta"].to_int64() != 0;
           KEYBINDS[kb.id] = kb;
         }
       }},
  };

  auto value = IO::ReadValue();
  if (!value) return false;

  auto type = Action::Type(value.value());
  auto data = Action::Data(value.value());

  auto it = HANDLERS.find(type.value_or(""));
  if (it != HANDLERS.end()) {
    it->second(data.value_or(Hjson::Value{}));
  } else {
    Log::Print("Main", "Unknown action type: %s", type.value_or("<none>").c_str());
  }
  return true;
}

}  // namespace Actions
