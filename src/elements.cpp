#include "elements.hpp"

#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "actions.hpp"
#include "hjson.h"
#include "imgui.h"
#include "imhtml.hpp"
#include "io.hpp"
#include "log.hpp"
#include "nfd.h"
#include "state.hpp"

namespace Elements {
namespace {

/**
 * @brief Safely parses a float from a string, returning a fallback on failure.
 *
 * @param s        The string to parse.
 * @param fallback Value returned when s is empty or not a valid float.
 * @return Parsed float, or fallback.
 */
float SafeStof(const std::string &s, float fallback = 0.0f) {
  if (s.empty()) return fallback;
  try {
    return std::stof(s);
  } catch (...) {
    return fallback;
  }
}

/**
 * @brief Parses a #RRGGBB or #RRGGBBAA hex string into an ImVec4 (r,g,b,a in [0,1]).
 *        Returns the fallback when the string is empty or malformed.
 *
 * @param hex      Hex color string.
 * @param fallback Fallback color.
 * @return Parsed color or fallback.
 */
ImVec4 HexToImVec4(const std::string &hex, ImVec4 fallback = ImVec4(0, 0, 0, 1)) {
  if (hex.empty() || hex[0] != '#') return fallback;
  unsigned int v = 0;
  if (hex.size() == 7 && sscanf(hex.c_str(), "#%06x", &v) == 1) {
    return ImVec4(((v >> 16) & 0xFF) / 255.0f, ((v >> 8) & 0xFF) / 255.0f, (v & 0xFF) / 255.0f, 1.0f);
  }
  if (hex.size() == 9 && sscanf(hex.c_str(), "#%08x", &v) == 1) {
    return ImVec4(
        ((v >> 24) & 0xFF) / 255.0f, ((v >> 16) & 0xFF) / 255.0f, ((v >> 8) & 0xFF) / 255.0f, (v & 0xFF) / 255.0f);
  }
  return fallback;
}

/**
 * @brief Converts an ImVec4 (r,g,b,a in [0,1]) to a #RRGGBB or #RRGGBBAA hex string.
 *
 * @param c     The color to convert.
 * @param alpha Whether to include the alpha channel.
 * @return Hex color string.
 */
std::string ImVec4ToHex(const ImVec4 &c, bool alpha) {
  auto clamp = [](float v) { return static_cast<int>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f); };
  char buf[10];
  if (alpha) {
    snprintf(buf, sizeof(buf), "#%02x%02x%02x%02x", clamp(c.x), clamp(c.y), clamp(c.z), clamp(c.w));
  } else {
    snprintf(buf, sizeof(buf), "#%02x%02x%02x", clamp(c.x), clamp(c.y), clamp(c.z));
  }
  return std::string(buf);
}

/**
 * @brief Seeds ELEMENT_STATE for an element if it has not been seen before.
 *
 * @param id         The element id.
 * @param attributes The element's HTML attributes.
 * @param valueAttr  The attribute name to use as the initial value (e.g. "value" or "checked").
 * @param fallback   Value to use when the attribute is absent.
 */
void SeedState(const std::string &id, const std::map<std::string, std::string> &attributes,
               const std::string &valueAttr = "value", const std::string &fallback = "") {
  if (!id.empty() && ELEMENT_STATE.find(id) == ELEMENT_STATE.end()) {
    auto it = attributes.find(valueAttr);
    ELEMENT_STATE[id] = (it != attributes.end()) ? it->second : fallback;
  }
}

/**
 * @brief Renders a label + Browse button for file/folder picker elements.
 *
 * On the frame after the button is clicked, invokes @p openDialog on the
 * main thread (required by NFD's Cocoa backend), stores the result in
 * ELEMENT_STATE and emits an input-changed event.
 *
 * @param id         Element id.
 * @param label      Button label text.
 * @param bounds     Layout bounds supplied by ImHTML.
 * @param openDialog Callable that invokes the appropriate NFD function and
 *                   returns the result code, writing the chosen path into
 *                   the provided nfdchar_t* output parameter.
 */
void RenderPathPicker(const std::string &id, const std::string &label, ImRect bounds,
                      std::function<nfdresult_t(nfdchar_t **)> openDialog) {
  static std::unordered_map<std::string, bool> PENDING;

  if (PENDING[id]) {
    PENDING[id] = false;
    nfdchar_t *outPath = nullptr;
    nfdresult_t result = openDialog(&outPath);
    if (result == NFD_OKAY && outPath) {
      ELEMENT_STATE[id] = std::string(outPath);
      Actions::EmitInputChanged(id, ELEMENT_STATE[id]);
      free(outPath);
    } else if (result == NFD_ERROR) {
      Log::Print("NFD", "Dialog error: %s", NFD_GetError());
    }
  }

  ImGui::SetCursorScreenPos(bounds.Min);
  ImGui::PushID(id.c_str());

  const std::string &current = ELEMENT_STATE[id];
  ImGui::TextUnformatted(current.empty() ? "(none)" : current.c_str());
  ImGui::SameLine();

  float btnW = std::min(bounds.GetWidth(), 90.0f);
  if (ImGui::Button(label.c_str(), ImVec2(btnW, bounds.GetHeight()))) {
    PENDING[id] = true;
  }

  ImGui::PopID();
}

}  // namespace

/**
 * @brief Registers all ui-* custom elements with ImHTML.
 */
void RegisterAll() {
  // ── ui-button ────────────────────────────────────────────────────────────
  // <ui-button text="..." action="..." id="..." tooltip="...">
  ImHTML::RegisterCustomElement("ui-button", [](ImRect bounds, std::map<std::string, std::string> attributes) {
    const std::string text = attributes["text"];
    const std::string label = attributes.count("id") ? (text + "##" + attributes["id"]) : text;

    ImGui::SetCursorScreenPos(bounds.Min);
    if (ImGui::Button(label.c_str(), bounds.GetSize())) {
      Hjson::Value msg;
      msg["action"] = "button-clicked";
      msg["data"]["pane"] = CURRENT_PANE;
      for (const auto &[key, value] : attributes) {
        msg["data"][key] = value;
      }
      IO::WriteValue(msg);
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
      if (attributes.count("tooltip")) {
        ImGui::SetTooltip("%s", attributes.at("tooltip").c_str());
      }
    }
  });

  // ── ui-input ─────────────────────────────────────────────────────────────
  // <ui-input id="..." type="text|number|int|password" placeholder="..." min="..." max="..." step="...">
  ImHTML::RegisterCustomElement("ui-input", [](ImRect bounds, std::map<std::string, std::string> attributes) {
    const std::string id = attributes["id"];
    const std::string type = attributes.count("type") ? attributes["type"] : "text";
    const std::string placeholder = attributes.count("placeholder") ? attributes["placeholder"] : "";

    SeedState(id, attributes);

    ImGui::SetCursorScreenPos(bounds.Min);
    ImGui::SetNextItemWidth(bounds.GetWidth());
    ImGui::PushID(id.c_str());

    const bool live = attributes.count("live") && attributes.at("live") == "true";

    if (type == "int") {
      float min = attributes.count("min") ? SafeStof(attributes["min"]) : 0.0f;
      float max = attributes.count("max") ? SafeStof(attributes["max"], 100.0f) : 100.0f;
      int step = attributes.count("step") ? static_cast<int>(SafeStof(attributes["step"], 1.0f)) : 1;
      int val = ELEMENT_STATE.count(id) ? static_cast<int>(SafeStof(ELEMENT_STATE[id])) : static_cast<int>(min);
      if (ImGui::InputInt("##input", &val, step, step * 10)) {
        val = static_cast<int>(std::clamp(static_cast<float>(val), min, max));
      }
      if (live && ImGui::IsItemActive()) {
        std::string newVal = std::to_string(val);
        if (!ELEMENT_STATE.count(id) || ELEMENT_STATE[id] != newVal) {
          Actions::EmitInputChanged(id, newVal);
        }
      } else if (!live && ImGui::IsItemDeactivatedAfterEdit()) {
        Actions::EmitInputChanged(id, std::to_string(val));
      }
    } else if (type == "number") {
      float min = attributes.count("min") ? SafeStof(attributes["min"]) : 0.0f;
      float max = attributes.count("max") ? SafeStof(attributes["max"], 100.0f) : 100.0f;
      float step = attributes.count("step") ? SafeStof(attributes["step"], 0.0f) : 0.0f;
      float val = ELEMENT_STATE.count(id) ? SafeStof(ELEMENT_STATE[id]) : 0.0f;
      if (ImGui::InputFloat("##input", &val, step, step * 10.0f, "%.4g")) {
        val = std::clamp(val, min, max);
      }
      if (live && ImGui::IsItemActive()) {
        std::string newVal = std::to_string(val);
        if (!ELEMENT_STATE.count(id) || ELEMENT_STATE[id] != newVal) {
          Actions::EmitInputChanged(id, newVal);
        }
      } else if (!live && ImGui::IsItemDeactivatedAfterEdit()) {
        Actions::EmitInputChanged(id, std::to_string(val));
      }
    } else if (type == "password") {
      char buf[1024] = {};
      if (ELEMENT_STATE.count(id)) {
        strncpy(buf, ELEMENT_STATE[id].c_str(), sizeof(buf) - 1);
      }
      ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_Password;
      if (live) flags |= ImGuiInputTextFlags_CallbackEdit;

      // Capture id for use in the callback lambda.
      const std::string capturedId = id;
      bool committed = ImGui::InputTextWithHint(
          "##input", placeholder.c_str(), buf, sizeof(buf), flags,
          live ? [](ImGuiInputTextCallbackData *data) -> int {
            const std::string &eid = *static_cast<const std::string *>(data->UserData);
            Actions::EmitInputChanged(eid, std::string(data->Buf, data->BufTextLen));
            return 0;
          }
               : static_cast<ImGuiInputTextCallback>(nullptr),
          live ? const_cast<void *>(static_cast<const void *>(&capturedId)) : nullptr);
      if (!live && (committed || ImGui::IsItemDeactivatedAfterEdit())) {
        Actions::EmitInputChanged(id, std::string(buf));
      }
    } else {
      // type == "text" (default)
      char buf[1024] = {};
      if (ELEMENT_STATE.count(id)) {
        strncpy(buf, ELEMENT_STATE[id].c_str(), sizeof(buf) - 1);
      }
      ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
      if (live) flags |= ImGuiInputTextFlags_CallbackEdit;

      const std::string capturedId = id;
      bool committed = ImGui::InputTextWithHint(
          "##input", placeholder.c_str(), buf, sizeof(buf), flags,
          live ? [](ImGuiInputTextCallbackData *data) -> int {
            const std::string &eid = *static_cast<const std::string *>(data->UserData);
            Actions::EmitInputChanged(eid, std::string(data->Buf, data->BufTextLen));
            return 0;
          }
               : static_cast<ImGuiInputTextCallback>(nullptr),
          live ? const_cast<void *>(static_cast<const void *>(&capturedId)) : nullptr);
      if (!live && (committed || ImGui::IsItemDeactivatedAfterEdit())) {
        Actions::EmitInputChanged(id, std::string(buf));
      }
    }

    ImGui::PopID();
  });

  // ── ui-select ────────────────────────────────────────────────────────────
  // <ui-select id="..." options="A|B|C" value="A">
  // Options are separated by "|". Use "\|" to include a literal pipe character.
  ImHTML::RegisterCustomElement("ui-select", [](ImRect bounds, std::map<std::string, std::string> attributes) {
    const std::string id = attributes["id"];

    std::vector<std::string> options;
    {
      const std::string &raw = attributes.count("options") ? attributes["options"] : "";
      std::string token;
      for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '\\' && i + 1 < raw.size() && raw[i + 1] == '|') {
          token += '|';
          ++i;
        } else if (raw[i] == '|') {
          options.push_back(token);
          token.clear();
        } else {
          token += raw[i];
        }
      }
      if (!token.empty() || !raw.empty()) {
        options.push_back(token);
      }
    }
    if (options.empty()) return;

    SeedState(id, attributes, "value", options[0]);

    int currentIndex = 0;
    for (int i = 0; i < static_cast<int>(options.size()); i++) {
      if (options[i] == ELEMENT_STATE[id]) {
        currentIndex = i;
        break;
      }
    }

    std::string comboLabel;
    for (const auto &opt : options) {
      comboLabel += opt;
      comboLabel += '\0';
    }
    comboLabel += '\0';

    ImGui::SetCursorScreenPos(bounds.Min);
    ImGui::SetNextItemWidth(bounds.GetWidth());
    ImGui::PushID(id.c_str());

    if (ImGui::Combo("##select", &currentIndex, comboLabel.c_str())) {
      Actions::EmitInputChanged(id, options[currentIndex]);
    }

    ImGui::PopID();
  });

  // ── ui-slider ────────────────────────────────────────────────────────────
  // <ui-slider id="..." min="0" max="1" value="0.5" type="float|int" format="...">
  ImHTML::RegisterCustomElement("ui-slider", [](ImRect bounds, std::map<std::string, std::string> attributes) {
    const std::string id = attributes["id"];
    const bool isInt = attributes.count("type") && attributes["type"] == "int";
    float min = attributes.count("min") ? SafeStof(attributes["min"]) : 0.0f;
    float max = attributes.count("max") ? SafeStof(attributes["max"], 1.0f) : 1.0f;

    SeedState(id, attributes, "value", std::to_string(isInt ? static_cast<int>(min) : min));

    ImGui::SetCursorScreenPos(bounds.Min);
    ImGui::SetNextItemWidth(bounds.GetWidth());
    ImGui::PushID(id.c_str());

    if (isInt) {
      const char *fmt = attributes.count("format") ? attributes.at("format").c_str() : "%d";
      int ival = ELEMENT_STATE.count(id) ? static_cast<int>(SafeStof(ELEMENT_STATE[id], min)) : static_cast<int>(min);
      int imin = static_cast<int>(min);
      int imax = static_cast<int>(max);
      ImGui::SliderInt("##slider", &ival, imin, imax, fmt);
      if (ImGui::IsItemActive()) {
        ELEMENT_STATE[id] = std::to_string(ival);
      }
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        Actions::EmitInputChanged(id, std::to_string(ival));
      }
    } else {
      const char *fmt = attributes.count("format") ? attributes.at("format").c_str() : "%.3f";
      float val = ELEMENT_STATE.count(id) ? SafeStof(ELEMENT_STATE[id], min) : min;
      ImGui::SliderFloat("##slider", &val, min, max, fmt);
      if (ImGui::IsItemActive()) {
        ELEMENT_STATE[id] = std::to_string(val);
      }
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        Actions::EmitInputChanged(id, std::to_string(val));
      }
    }

    ImGui::PopID();
  });

  // ── ui-checkbox ──────────────────────────────────────────────────────────
  // <ui-checkbox id="..." label="..." checked="true|false">
  ImHTML::RegisterCustomElement("ui-checkbox", [](ImRect bounds, std::map<std::string, std::string> attributes) {
    const std::string id = attributes["id"];
    const std::string label = attributes.count("label") ? attributes["label"] : "";

    SeedState(id, attributes, "checked", "false");

    bool checked = ELEMENT_STATE.count(id) && ELEMENT_STATE[id] == "true";

    ImGui::SetCursorScreenPos(bounds.Min);
    ImGui::PushID(id.c_str());

    if (ImGui::Checkbox(label.c_str(), &checked)) {
      Actions::EmitInputChanged(id, checked ? "true" : "false");
    }

    ImGui::PopID();
  });

  // ── ui-textarea ──────────────────────────────────────────────────────────
  // <ui-textarea id="..." placeholder="..." value="...">
  ImHTML::RegisterCustomElement("ui-textarea", [](ImRect bounds, std::map<std::string, std::string> attributes) {
    const std::string id = attributes["id"];
    const std::string placeholder = attributes.count("placeholder") ? attributes["placeholder"] : "";
    const bool live = attributes.count("live") && attributes.at("live") == "true";

    SeedState(id, attributes);

    ImGui::SetCursorScreenPos(bounds.Min);
    ImGui::PushID(id.c_str());

    static std::unordered_map<std::string, std::string> BUFFERS;
    if (BUFFERS.find(id) == BUFFERS.end() || BUFFERS[id] != ELEMENT_STATE[id]) {
      BUFFERS[id] = ELEMENT_STATE[id];
    }

    std::string &buf = BUFFERS[id];
    buf.resize(std::max(buf.size(), static_cast<size_t>(4096)), '\0');

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
    if (live) flags |= ImGuiInputTextFlags_CallbackEdit;

    const std::string capturedId = id;
    if (ImGui::InputTextMultiline(
            "##textarea", buf.data(), buf.size(), bounds.GetSize(), flags,
            live ? [](ImGuiInputTextCallbackData *data) -> int {
              const std::string &eid = *static_cast<const std::string *>(data->UserData);
              Actions::EmitInputChanged(eid, std::string(data->Buf, data->BufTextLen));
              return 0;
            }
                 : static_cast<ImGuiInputTextCallback>(nullptr),
            live ? const_cast<void *>(static_cast<const void *>(&capturedId)) : nullptr)) {
      buf.resize(strlen(buf.data()));
    }
    if (!live && ImGui::IsItemDeactivatedAfterEdit()) {
      buf.resize(strlen(buf.data()));
      Actions::EmitInputChanged(id, buf);
    }

    ImGui::PopID();
  });

  // ── ui-progress ──────────────────────────────────────────────────────────
  // <ui-progress id="..." value="0.5" min="0" max="1" overlay="50%">
  ImHTML::RegisterCustomElement("ui-progress", [](ImRect bounds, std::map<std::string, std::string> attributes) {
    const std::string id = attributes["id"];

    SeedState(id, attributes, "value", "0");

    float min = attributes.count("min") ? SafeStof(attributes["min"]) : 0.0f;
    float max = attributes.count("max") ? SafeStof(attributes["max"], 1.0f) : 1.0f;
    float val = ELEMENT_STATE.count(id) ? SafeStof(ELEMENT_STATE[id], min) : min;
    float fraction = (max > min) ? std::clamp((val - min) / (max - min), 0.0f, 1.0f) : 0.0f;

    const char *overlay = attributes.count("overlay") ? attributes.at("overlay").c_str() : nullptr;

    ImGui::SetCursorScreenPos(bounds.Min);
    ImGui::ProgressBar(fraction, bounds.GetSize(), overlay);
  });

  // ── ui-color ─────────────────────────────────────────────────────────────
  // <ui-color id="..." value="#rrggbb" alpha="false">
  ImHTML::RegisterCustomElement("ui-color", [](ImRect bounds, std::map<std::string, std::string> attributes) {
    const std::string id = attributes["id"];
    const bool alpha = attributes.count("alpha") && attributes["alpha"] == "true";

    SeedState(id, attributes, "value", alpha ? "#000000ff" : "#000000");

    ImVec4 col = HexToImVec4(ELEMENT_STATE.count(id) ? ELEMENT_STATE[id] : "#000000");

    ImGui::SetCursorScreenPos(bounds.Min);
    ImGui::SetNextItemWidth(bounds.GetWidth());
    ImGui::PushID(id.c_str());

    ImGuiColorEditFlags flags = ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_InputRGB;
    if (!alpha) flags |= ImGuiColorEditFlags_NoAlpha;

    bool changed = alpha ? ImGui::ColorEdit4("##color", reinterpret_cast<float *>(&col), flags)
                         : ImGui::ColorEdit3("##color", reinterpret_cast<float *>(&col), flags);
    if (changed) {
      std::string hex = ImVec4ToHex(col, alpha);
      ELEMENT_STATE[id] = hex;
      Actions::EmitInputChanged(id, hex);
    }

    ImGui::PopID();
  });

  // ── ui-file-select ───────────────────────────────────────────────────────
  // <ui-file-select id="..." label="..." filter="png,jpg" value="...">
  ImHTML::RegisterCustomElement("ui-file-select", [](ImRect bounds, std::map<std::string, std::string> attributes) {
    const std::string id = attributes["id"];
    const std::string label = attributes.count("label") ? attributes["label"] : "Browse...";
    const std::string filter = attributes.count("filter") ? attributes["filter"] : "";

    SeedState(id, attributes);

    RenderPathPicker(id, label, bounds, [filter](nfdchar_t **out) {
      const char *filterPtr = filter.empty() ? nullptr : filter.c_str();
      return NFD_OpenDialog(filterPtr, nullptr, out);
    });
  });

  // ── ui-folder-select ─────────────────────────────────────────────────────
  // <ui-folder-select id="..." label="..." value="...">
  ImHTML::RegisterCustomElement("ui-folder-select", [](ImRect bounds, std::map<std::string, std::string> attributes) {
    const std::string id = attributes["id"];
    const std::string label = attributes.count("label") ? attributes["label"] : "Browse...";

    SeedState(id, attributes);

    RenderPathPicker(id, label, bounds, [](nfdchar_t **out) { return NFD_PickFolder(nullptr, out); });
  });

  // ── ui-file-save ─────────────────────────────────────────────────────────
  // <ui-file-save id="..." label="..." filter="png,jpg" value="...">
  ImHTML::RegisterCustomElement("ui-file-save", [](ImRect bounds, std::map<std::string, std::string> attributes) {
    const std::string id = attributes["id"];
    const std::string label = attributes.count("label") ? attributes["label"] : "Save...";
    const std::string filter = attributes.count("filter") ? attributes["filter"] : "";

    SeedState(id, attributes);

    RenderPathPicker(id, label, bounds, [filter](nfdchar_t **out) {
      const char *filterPtr = filter.empty() ? nullptr : filter.c_str();
      return NFD_SaveDialog(filterPtr, nullptr, out);
    });
  });
}

}  // namespace Elements
