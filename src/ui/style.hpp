#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "hjson.h"
#include "imgui.h"

/**
 * @namespace UI::Style
 * @brief Styling and typography helpers for ImGui.
 */
namespace UI::Style {
/**
 * @brief Font style enum
 */
enum class FontStyle : unsigned char { Regular, Bold, Italic, BoldItalic };

/**
 * @brief Structure to hold a color scheme
 */
struct ColorScheme {
  ImVec4 text;
  ImVec4 textMuted;
  ImVec4 windowBg;
  ImVec4 elementBg;
  ImVec4 elementHovered;
  ImVec4 elementActive;
  ImVec4 titleBg;
  ImVec4 border;
  ImVec4 primary;
  ImVec4 warn;
  ImVec4 danger;
  ImVec4 primaryTranslucent;
  ImVec4 primarySubtle;
  ImVec4 textSelectionBg;
  ImVec4 modalDim;
};

/**
 * @brief Sets up ImGui style
 * @param onlyColors If true, only reloads colors without reinitializing fonts and other settings
 */
void SetupStyle(bool onlyColors = false);

/**
 * @brief Gets currently active color scheme with fallback to default.
 * @return Active color scheme reference.
 */
const ColorScheme& GetCurrentColorScheme();

/**
 * @brief Loads a color scheme from a Hjson value with fallback to default.
 * @param root Hjson value containing color scheme properties.
 * @return Loaded color scheme.
 */
ColorScheme LoadTheme(Hjson::Value root);

/**
 * @brief Returns ImGui font
 */
ImFont* GetFont(FontStyle style = FontStyle::Regular);

/**
 * @brief Returns a ImGui font size based on steps
 * @param size The step size, with 0 == base size and negative getting smaller and positive getting larger
 */
float GetSize(int size);

/**
 * @brief Returns configured base font size in pixels.
 * @return Base font size.
 */
float GetBaseFontSize();

/**
 * @brief Returns clamped configured base font size.
 * @return Clamped base font size.
 */
float ClampedConfiguredBaseSize();

}  // namespace UI::Style
