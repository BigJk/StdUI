#include "renderer.hpp"

#include <algorithm>
#include <string>

#include "actions.hpp"
#include "audio.hpp"
#include "hjson.h"
#include "imgui.h"
#include "imhtml.hpp"
#include "io.hpp"
#include "layout.hpp"
#include "raylib.h"
#include "rlImGui.h"
#include "rlgl.h"
#include "state.hpp"
#include "texture_cache.hpp"
#include "ui/style.hpp"

namespace Renderer {

/**
 * @brief Builds the styled HTML string to feed into ImHTML::Canvas each frame.
 *
 * Prepends a <style> block that sets the base font size and correct intrinsic
 * heights for all ui-* custom elements, then appends the given content string.
 *
 * @param content The raw HTML content to wrap with element styles.
 * @return The complete HTML string ready for rendering.
 */
std::string BuildElementStyles(const std::string &content) {
  // ui-textarea defaults to 4 rows: line height + item spacing, plus frame padding.
  float lineH = ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y;
  int textareaHeight = static_cast<int>(lineH * 4 + ImGui::GetStyle().FramePadding.y * 2);
  int widgetHeight = static_cast<int>(ImGui::GetFrameHeight());

  return "<style>"
         "html{font-size:" +
         std::to_string(static_cast<int>(UI::Style::GetBaseFontSize())) +
         "px;}"
         "ui-input,ui-select,ui-slider,ui-checkbox,ui-color{display:block;height:" +
         std::to_string(widgetHeight) +
         "px}"
         "ui-textarea{display:block;height:" +
         std::to_string(textareaHeight) +
         "px}"
         "ui-progress{display:block;height:" +
         std::to_string(widgetHeight) +
         "px}"
         "ui-file-select,ui-folder-select,ui-file-save,ui-button{display:block;min-height:" +
         std::to_string(widgetHeight) +
         "px}"
         "</style>" +
         content;
}

/**
 * @brief Recursively renders a layout node into ImGui windows.
 *
 * For leaf pane nodes an ImGui child window is created at the given origin
 * with the given size, and the pane's HTML content is rendered inside it.
 * For split nodes the available space is partitioned among children according
 * to their relative size weights, then each child is rendered recursively.
 *
 * @param node   The layout node to render.
 * @param origin Top-left position in screen coordinates.
 * @param size   Available width and height in pixels.
 */
void RenderNode(const Layout::Node &node, ImVec2 origin, ImVec2 size) {
  if (node.type == Layout::Node::Type::Pane) {
    // Create a borderless, decoration-free child window for this pane.
    ImGui::SetNextWindowPos(origin, ImGuiCond_Always);
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    std::string windowId = "pane##" + node.id;
    ImGui::Begin(windowId.c_str(),
                 nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PopStyleVar();

    // Apply pending scroll requests after Begin() so the document height from
    // the previous frame is already committed to ImGui's scroll state.
    // A countdown of > 0 means "wait N more frames" (defers past content
    // updates that arrive in the same batch).
    auto scrollIt = SCROLL_REQUESTS.find(node.id);
    if (scrollIt != SCROLL_REQUESTS.end()) {
      auto &req = scrollIt->second;
      if (req.framesLeft > 0) {
        --req.framesLeft;
      } else {
        float y = (req.target < 0.0f) ? ImGui::GetScrollMaxY() : req.target;
        ImGui::SetScrollY(y);
        SCROLL_REQUESTS.erase(scrollIt);
      }
    }

    auto it = PANE_CONTENT.find(node.id);
    const std::string &raw = (it != PANE_CONTENT.end()) ? it->second : std::string();
    std::string styledContent = BuildElementStyles(raw);

    std::string canvasId = "canvas##" + node.id;
    std::string clickedURL;
    CURRENT_PANE = node.id;
    if (ImHTML::Canvas(canvasId.c_str(), styledContent.c_str(), 0.0f, &clickedURL)) {
      Hjson::Value msg;
      msg["action"] = "url-clicked";
      msg["data"]["url"] = clickedURL;
      msg["data"]["pane"] = node.id;
      IO::WriteValue(msg);
    }
    CURRENT_PANE = "";

    ImGui::End();
    return;
  }

  // Split node — partition available space among children.
  // size > 10  → fixed pixel allocation.
  // size <= 10 → relative weight; shares the space left after fixed children.
  if (node.children.empty()) {
    return;
  }

  const float totalAxis = (node.direction == Layout::Direction::Horizontal) ? size.x : size.y;

  // Pass 1: sum up fixed pixels and relative weights separately.
  float fixedTotal = 0.0f;
  float weightTotal = 0.0f;
  for (const auto &child : node.children) {
    float s = (child->size > 0.0f ? child->size : 1.0f);
    if (s > 10.0f) {
      fixedTotal += s;
    } else {
      weightTotal += s;
    }
  }

  float flexSpace = std::max(0.0f, totalAxis - fixedTotal);

  float cursor = (node.direction == Layout::Direction::Horizontal) ? origin.x : origin.y;
  float end = (node.direction == Layout::Direction::Horizontal) ? origin.x + size.x : origin.y + size.y;

  for (size_t i = 0; i < node.children.size(); ++i) {
    const auto &child = node.children[i];
    bool isLast = (i == node.children.size() - 1);

    float s = (child->size > 0.0f ? child->size : 1.0f);
    float alloc;
    if (s > 10.0f) {
      alloc = s;  // fixed pixels
    } else {
      alloc = (weightTotal > 0.0f) ? (flexSpace * s / weightTotal) : 0.0f;
    }
    // Last child absorbs any rounding remainder to prevent gaps.
    if (isLast) alloc = end - cursor + 1.0f;

    ImVec2 childOrigin;
    ImVec2 childSize;

    if (node.direction == Layout::Direction::Horizontal) {
      childOrigin = ImVec2(cursor, origin.y);
      childSize = ImVec2(alloc, size.y);
    } else {
      childOrigin = ImVec2(origin.x, cursor);
      childSize = ImVec2(size.x, alloc);
    }

    cursor += alloc;
    RenderNode(*child, childOrigin, childSize);
  }
}

/**
 * @brief Executes one iteration of the main render loop.
 *
 * Drains all pending IO actions, processes file drops, ticks audio,
 * syncs the ImHTML base font size, then draws the full frame.
 */
void MainLoopIteration() {
  // Drain the entire action queue — process every pending message before
  // rendering so that burst updates (e.g. initialising multiple panes at
  // once) are visible in the same frame rather than trickling in one per
  // frame at 60 Hz.
  while (Actions::Process()) {
  }

  //
  // Keybind polling — check every registered shortcut and emit key-pressed
  // when all required modifiers are held and the primary key is pressed this
  // frame.  Modifier matching uses logical OR of left/right variants so the
  // user does not need to hold a specific side.
  //
  if (!KEYBINDS.empty()) {
    bool ctrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    bool altDown = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
    bool metaDown = IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);

    for (const auto &[id, kb] : KEYBINDS) {
      if (kb.ctrl && !ctrlDown) continue;
      if (kb.shift && !shiftDown) continue;
      if (kb.alt && !altDown) continue;
      if (kb.meta && !metaDown) continue;
      if (!IsKeyPressed(kb.key)) continue;

      Hjson::Value msg;
      msg["action"] = "key-pressed";
      msg["data"]["id"] = kb.id;
      IO::WriteValue(msg);
    }
  }

  //
  // File drop — emit one file-dropped event per dropped file.
  //
  if (IsFileDropped()) {
    FilePathList dropped = LoadDroppedFiles();
    for (unsigned int i = 0; i < dropped.count; i++) {
      Hjson::Value msg;
      msg["action"] = "file-dropped";
      msg["data"]["path"] = dropped.paths[i];
      IO::WriteValue(msg);
    }
    UnloadDroppedFiles(dropped);
  }

  //
  // Audio
  //
  Audio::Update();

  //
  // Sync ImHTML base font size — must be set before ImHTML::Canvas().
  //
  ImHTML::GetConfig()->BaseFontSize = UI::Style::GetBaseFontSize();

  //
  // Final Drawing
  //
  BeginDrawing();
  {
#ifdef __APPLE__
    // Fix ES 2.0 blend mode
    rlSetBlendFactorsSeparate(0x0302, 0x0303, 1, 0x0303, 0x8006, 0x8006);
    BeginBlendMode(BLEND_CUSTOM_SEPARATE);
#endif

    ClearBackground(Color{0, 0, 0, 255});

    rlImGuiBegin();
    {
      ImVec2 displaySize = ImGui::GetIO().DisplaySize;
      RenderNode(*LAYOUT, ImVec2(0, 0), displaySize);

      //
      // Confirm modal — opened whenever CONFIRM_PENDING is set.
      //
      if (CONFIRM_PENDING.has_value()) {
        ImGui::OpenPopup("##confirm");
      }

      ImVec2 center = ImGui::GetMainViewport()->GetCenter();
      ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
      if (ImGui::BeginPopupModal(
              "##confirm",
              nullptr,
              ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar)) {
        if (CONFIRM_PENDING.has_value()) {
          const ConfirmDialog &dlg = CONFIRM_PENDING.value();

          // Title
          ImGui::TextUnformatted(dlg.title.c_str());
          ImGui::Separator();
          ImGui::Spacing();

          // Question
          ImGui::TextUnformatted(dlg.question.c_str());
          ImGui::Spacing();

          // Buttons
          bool emitResult = false;
          bool result = false;

          if (ImGui::Button(dlg.okText.c_str())) {
            emitResult = true;
            result = true;
          }
          ImGui::SameLine();
          if (ImGui::Button(dlg.cancelText.c_str())) {
            emitResult = true;
            result = false;
          }

          if (emitResult) {
            Hjson::Value msg;
            msg["action"] = "confirm-result";
            msg["data"]["id"] = dlg.id;
            msg["data"]["result"] = result;
            IO::WriteValue(msg);
            CONFIRM_PENDING.reset();
            ImGui::CloseCurrentPopup();
          }
        }
        ImGui::EndPopup();
      }
      //
      // Toasts — decoration-free windows stacked top-center, TTL + double-click dismiss.
      //
      {
        double now = GetTime();
        // Erase expired or dismissed toasts.
        TOASTS.erase(std::remove_if(TOASTS.begin(),
                                    TOASTS.end(),
                                    [now](const Toast &t) {
                                      return t.dismissed || (now - t.createdAt) >= static_cast<double>(t.ttl);
                                    }),
                     TOASTS.end());

        // Stack toasts downward from the top-center with a small gap.
        const float GAP = 8.0f;
        const float TOP_MARGIN = 12.0f;
        float yOffset = TOP_MARGIN;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        for (size_t i = 0; i < TOASTS.size(); ++i) {
          Toast &toast = TOASTS[i];

          float posX = (displaySize.x - toast.width) * 0.5f;
          float posY = yOffset;

          ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
          ImGui::SetNextWindowSize(ImVec2(toast.width, toast.height), ImGuiCond_Always);
          ImGui::SetNextWindowBgAlpha(0.0f);

          std::string toastId = "toast_" + std::to_string(i);
          ImGui::Begin(toastId.c_str(),
                       nullptr,
                       ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                           ImGuiWindowFlags_AlwaysAutoResize);
          // Double-click anywhere inside the toast window dismisses it.
          if (ImGui::IsWindowHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            toast.dismissed = true;
          }

          std::string styledContent = BuildElementStyles(toast.content);
          std::string canvasId = "toast_canvas##" + std::to_string(i);
          ImHTML::Canvas(canvasId.c_str(), styledContent.c_str(), 0.0f, nullptr);

          yOffset += ImGui::GetWindowHeight() + GAP;
          ImGui::End();
        }
        ImGui::PopStyleVar(2);
      }
    }
    rlImGuiEnd();
  }
  EndDrawing();
}

}  // namespace Renderer
