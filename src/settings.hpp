#pragma once

#include <string>

#include "hjson.h"
#include "ui/style.hpp"

namespace Settings {

/**
 * @brief Structure to hold all the settings data.
 */
struct Data {
  std::string title;                   ///< Window title
  bool resizable;                      ///< Whether the window is resizable
  int windowWidth;                     ///< Window width in pixels
  int windowHeight;                    ///< Window height in pixels
  int windowMinWidth;                  ///< Minimum window width in pixels
  int windowMinHeight;                 ///< Minimum window height in pixels
  int windowMaxWidth;                  ///< Maximum window width in pixels
  int windowMaxHeight;                 ///< Maximum window height in pixels
  int targetFps;                       ///< Target frame rate in frames per second
  float audioVolume;                   ///< Sound effects volume level
  float baseFontSize;                  ///< Base font size in pixels
  std::string fontRegular;             ///< Filename of the regular font
  std::string fontBold;                ///< Filename of the bold font
  std::string fontItalic;              ///< Filename of the italic font
  std::string fontBoldItalic;          ///< Filename of the bold italic font
  UI::Style::ColorScheme colorScheme;  ///< Name of the selected color scheme
};

/**
 * @brief Loads the settings from the specified data string (e.g., JSON or HJSON).
 * @param root The root Hjson::Value containing the settings data.
 */
void Load(Hjson::Value root);

/**
 * @brief Gets a pointer to the current settings data.
 *
 * @return Pointer to the settings data.
 */
Data* Get();

}  // namespace Settings
