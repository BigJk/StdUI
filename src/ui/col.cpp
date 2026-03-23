#include "col.hpp"

#include "imgui.h"
#include "ui/style.hpp"

namespace UI::Col {
ImVec4 Alpha(ImVec4 col, float alpha) { return ImVec4(col.x, col.y, col.z, alpha); }
ImVec4 Hex(int hex, float alpha) {
  ImVec4 color;
  color.x = ((hex >> 16) & 0xFF) / 255.0f;
  color.y = ((hex >> 8) & 0xFF) / 255.0f;
  color.z = ((hex)&0xFF) / 255.0f;
  color.w = alpha;
  return color;
}
}  // namespace UI::Col
