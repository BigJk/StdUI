#pragma once

#include "imgui.h"

namespace UI::Col {
/**
 * @brief Returns a color with the same RGB values but modified alpha.
 * @param col Base color.
 * @param alpha New alpha value (0.0f - 1.0f).
 * @return Color with modified alpha.
 */
ImVec4 Alpha(ImVec4 col, float alpha);

/**
 * @brief Parses a hex color code and returns an ImVec4.
 * @param hex Hex color code (e.g. 0xRRGGBB).
 * @param alpha Optional alpha value (0.0f - 1.0f), default is 1.0f.
 * @return Parsed color as ImVec4.
 */
ImVec4 Hex(int hex, float alpha = 1.0f);
}  // namespace UI::Col
