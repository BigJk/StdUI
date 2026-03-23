#include "./style.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "hjson.h"
#include "imgui.h"
#include "log.hpp"
#include "raylib.h"
#include "settings.hpp"
#include "ui/col.hpp"

// If the env var UI_HIGHDPI is set, we're on a high DPI display.
bool HighDPI() { return std::getenv("UI_HIGHDPI") != nullptr; }

namespace UI::Style {
namespace {
constexpr float DEFAULT_BASE_FONT_SIZE = 20.0f;
constexpr float MIN_BASE_FONT_SIZE = 10.0f;
constexpr float MAX_BASE_FONT_SIZE = 64.0f;
}  // namespace

/**
 * @brief Internal style helpers and cached font metadata.
 */
const ImWchar* GetFontGlyphRanges() {
  static ImVector<ImWchar> ranges;
  if (!ranges.empty()) {
    return ranges.Data;
  }

  ImFontGlyphRangesBuilder builder;
  builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesDefault());

  static const ImWchar nerdRanges[] = {
      (ImWchar)0xE000,
      (ImWchar)0xF8FF,  // BMP private use area
      (ImWchar)0xF0000,
      (ImWchar)0xF2FFF,  // Supplementary private use area
      0,
  };
  builder.AddRanges(nerdRanges);
  builder.BuildRanges(&ranges);
  return ranges.Data;
}

/**
 * @brief Returns default light theme scheme.
 * @return Default built-in color scheme.
 */
ColorScheme DefaultScheme() {
  return {
      .text = UI::Col::Hex(0x1f2328),
      .textMuted = UI::Col::Hex(0x656d76),
      .windowBg = UI::Col::Hex(0xffffff),
      .elementBg = UI::Col::Hex(0xf6f8fa),
      .elementHovered = UI::Col::Hex(0xeaeef2),
      .elementActive = UI::Col::Hex(0xd0d7de),
      .titleBg = UI::Col::Hex(0xf6f8fa),
      .border = UI::Col::Hex(0xd0d7de),
      .primary = UI::Col::Hex(0x0969da),
      .warn = UI::Col::Hex(0x9a6700),
      .danger = UI::Col::Hex(0xd1242f),
      .primaryTranslucent = UI::Col::Hex(0x0969da, 0.8f),
      .primarySubtle = UI::Col::Hex(0x0969da, 0.1f),
      .textSelectionBg = UI::Col::Hex(0x0969da, 0.2f),
      .modalDim = UI::Col::Hex(0x000000, 0.3f),
  };
}

/**
 * @brief Returns a contrasting text color based on background luminance.
 * @param bg Background color.
 * @return Dark text for light backgrounds, light text for dark backgrounds.
 */
ImVec4 AutoTextColor(const ImVec4& bg) {
  const float luminance = 0.2126f * bg.x + 0.7152f * bg.y + 0.0722f * bg.z;
  return luminance > 0.6f ? UI::Col::Hex(0x1f2430) : UI::Col::Hex(0xe0e0e0);
}

/**
 * @brief Returns a muted text variant from base text color.
 * @param text Base text color.
 * @return Muted text color.
 */
ImVec4 AutoMutedTextColor(const ImVec4& text) { return ImVec4(text.x * 0.7f, text.y * 0.7f, text.z * 0.7f, 1.0f); }

/**
 * @brief Parses #RRGGBB or #RRGGBBAA hex color strings.
 * @param value Hex string.
 * @param fallback Fallback color.
 * @return Parsed color or fallback.
 */
ImVec4 ParseHexColor(const std::string& value, const ImVec4& fallback) {
  if (value.empty() || value[0] != '#') {
    return fallback;
  }

  unsigned int rgba = 0;
  if (value.size() == 7) {
    if (sscanf(value.c_str(), "#%x", &rgba) == 1) {
      return ImVec4(((rgba >> 16) & 0xFF) / 255.0f, ((rgba >> 8) & 0xFF) / 255.0f, (rgba & 0xFF) / 255.0f, 1.0f);
    }
  } else if (value.size() == 9) {
    if (sscanf(value.c_str(), "#%x", &rgba) == 1) {
      return ImVec4(((rgba >> 24) & 0xFF) / 255.0f,
                    ((rgba >> 16) & 0xFF) / 255.0f,
                    ((rgba >> 8) & 0xFF) / 255.0f,
                    (rgba & 0xFF) / 255.0f);
    }
  }

  return fallback;
}

/**
 * @brief Reads one theme.hjson file.
 * @param themePath Path to theme.hjson.
 * @param fallback Fallback defaults.
 * @return Parsed color scheme.
 */
ColorScheme LoadTheme(Hjson::Value root) {
  ColorScheme fallback = DefaultScheme();
  ColorScheme scheme;

  scheme.windowBg = ParseHexColor(root["windowBg"].to_string(), fallback.windowBg);
  scheme.elementBg = ParseHexColor(root["elementBg"].to_string(), fallback.elementBg);
  scheme.elementHovered = ParseHexColor(root["elementHovered"].to_string(), fallback.elementHovered);
  scheme.elementActive = ParseHexColor(root["elementActive"].to_string(), fallback.elementActive);
  scheme.titleBg = ParseHexColor(root["titleBg"].to_string(), fallback.titleBg);
  scheme.border = ParseHexColor(root["border"].to_string(), fallback.border);
  scheme.primary = ParseHexColor(root["primary"].to_string(), fallback.primary);
  scheme.warn = ParseHexColor(root["warn"].to_string(), fallback.warn);
  scheme.danger = ParseHexColor(root["danger"].to_string(), fallback.danger);
  scheme.primaryTranslucent = ParseHexColor(root["primaryTranslucent"].to_string(), fallback.primaryTranslucent);
  scheme.primarySubtle = ParseHexColor(root["primarySubtle"].to_string(), fallback.primarySubtle);
  scheme.textSelectionBg = ParseHexColor(root["textSelectionBg"].to_string(), fallback.textSelectionBg);
  scheme.modalDim = ParseHexColor(root["modalDim"].to_string(), fallback.modalDim);
  scheme.text =
      root["text"].defined() ? ParseHexColor(root["text"].to_string(), fallback.text) : AutoTextColor(scheme.windowBg);
  scheme.textMuted = root["textMuted"].defined() ? ParseHexColor(root["textMuted"].to_string(), fallback.textMuted)
                                                 : AutoMutedTextColor(scheme.text);

  return scheme;
}

const ColorScheme& GetCurrentColorScheme() { return Settings::Get()->colorScheme; }

ImFont* InitFont(float size, FontStyle style) {
  auto settings = Settings::Get();

  std::string font;

  switch (style) {
    case FontStyle::Regular:
      font = settings->fontRegular;
      break;
    case FontStyle::Bold:
      font = settings->fontBold;
      break;
    case FontStyle::Italic:
      font = settings->fontItalic;
      break;
    case FontStyle::BoldItalic:
      font = settings->fontBoldItalic;
      break;
  }

  if (font.empty()) {
    font = "./assets/fonts/Iosevka-Regular.ttf";
  }

  Log::Print("Font", "[Font] Loading font (%s) for size %f", font.c_str(), size);

  return ImGui::GetIO().Fonts->AddFontFromFileTTF(font.c_str(), size, nullptr, GetFontGlyphRanges());
}

float ClampedConfiguredBaseSize() {
  const Settings::Data* settings = Settings::Get();
  float base = settings ? settings->baseFontSize : DEFAULT_BASE_FONT_SIZE;
  if (base < MIN_BASE_FONT_SIZE) {
    base = MIN_BASE_FONT_SIZE;
  }
  if (base > MAX_BASE_FONT_SIZE) {
    base = MAX_BASE_FONT_SIZE;
  }
  return base;
}

void SetupStyle(bool onlyColors) {
  auto style = &ImGui::GetStyle();

  const ColorScheme& scheme = GetCurrentColorScheme();

  // Apply color scheme
  // Window & Title
  style->Colors[ImGuiCol_Text] = scheme.text;
  style->Colors[ImGuiCol_TextDisabled] = scheme.textMuted;
  style->Colors[ImGuiCol_TitleBgActive] = scheme.titleBg;
  style->Colors[ImGuiCol_TitleBg] = scheme.titleBg;
  style->Colors[ImGuiCol_TitleBgCollapsed] = scheme.titleBg;
  style->Colors[ImGuiCol_WindowBg] = scheme.windowBg;
  style->Colors[ImGuiCol_PopupBg] = scheme.windowBg;
  style->Colors[ImGuiCol_Border] = scheme.border;

  // Buttons
  style->Colors[ImGuiCol_Button] = scheme.elementBg;
  style->Colors[ImGuiCol_ButtonHovered] = scheme.elementHovered;
  style->Colors[ImGuiCol_ButtonActive] = scheme.elementActive;

  // Frames (Input fields, etc.)
  style->Colors[ImGuiCol_FrameBg] = scheme.elementBg;
  style->Colors[ImGuiCol_FrameBgHovered] = scheme.primarySubtle;
  style->Colors[ImGuiCol_FrameBgActive] = scheme.elementHovered;

  // Headers
  style->Colors[ImGuiCol_Header] = scheme.elementBg;
  style->Colors[ImGuiCol_HeaderHovered] = scheme.elementHovered;
  style->Colors[ImGuiCol_HeaderActive] = scheme.elementActive;

  // Sliders
  style->Colors[ImGuiCol_SliderGrab] = scheme.primaryTranslucent;
  style->Colors[ImGuiCol_SliderGrabActive] = scheme.primary;

  // Resize Grips
  style->Colors[ImGuiCol_ResizeGrip] = scheme.border;
  style->Colors[ImGuiCol_ResizeGripHovered] = scheme.elementHovered;
  style->Colors[ImGuiCol_ResizeGripActive] = scheme.elementActive;

  // Other Elements
  style->Colors[ImGuiCol_CheckMark] = scheme.primary;
  style->Colors[ImGuiCol_TextLink] = scheme.primary;
  style->Colors[ImGuiCol_InputTextCursor] = scheme.text;
  style->Colors[ImGuiCol_TextSelectedBg] = scheme.textSelectionBg;
  style->Colors[ImGuiCol_ModalWindowDimBg] = scheme.modalDim;

  // Tabs
  style->Colors[ImGuiCol_Tab] = scheme.elementBg;
  style->Colors[ImGuiCol_TabHovered] = scheme.elementHovered;
  style->Colors[ImGuiCol_TabActive] = scheme.elementActive;
  style->Colors[ImGuiCol_TabUnfocused] = scheme.elementBg;
  style->Colors[ImGuiCol_TabUnfocusedActive] = scheme.elementBg;

  // Progress Bar
  style->Colors[ImGuiCol_PlotHistogram] = scheme.primary;
  style->Colors[ImGuiCol_PlotHistogramHovered] = scheme.primaryTranslucent;

  // Scrollbar
  style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
  style->Colors[ImGuiCol_ScrollbarGrab] = scheme.border;
  style->Colors[ImGuiCol_ScrollbarGrabHovered] = scheme.elementActive;
  style->Colors[ImGuiCol_ScrollbarGrabActive] = scheme.primary;

  ImGuiIO& io = ImGui::GetIO();

  // Only initialize other style settings and fonts if not just updating colors
  if (!onlyColors) {
    // Other style settings
    style->WindowRounding = 0.0f;
    style->FrameRounding = 4.0f;
    style->WindowBorderSize = 1.0f;
    style->PopupBorderSize = 1.0f;
    style->WindowPadding = ImVec2(8, 8);
    style->FramePadding = ImVec2(16, 8);
    style->ScrollbarSize = 10;
    style->ScrollbarRounding = 0;

    io.IniFilename = "";
    io.FontDefault = GetFont();
  }
}

ImFont* GetFont(FontStyle style) {
  static std::unordered_map<FontStyle, ImFont*> fonts = {};

  if (fonts.find(style) == fonts.end()) {
    fonts[style] = InitFont(GetSize(0), style);
  }

  return fonts[style];
}

float GetSize(int size) {
  float baseSize = ClampedConfiguredBaseSize();
  return baseSize + float(size - 1) * (2.0f * (size < 0 ? 0.5f : 1.0f));
}

float GetBaseFontSize() { return ClampedConfiguredBaseSize(); }

}  // namespace UI::Style
