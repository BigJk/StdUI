#include "settings.hpp"

#include "hjson.h"
#include "log.hpp"

namespace Settings {
static Data settingsData;

void Load(Hjson::Value root) {
  settingsData.title = root["title"].defined() ? root["title"].to_string() : "StdUI";
  settingsData.resizable = root["resizable"].defined() ? root["resizable"].to_int64() : true;
  settingsData.windowWidth = root["windowWidth"].defined() ? root["windowWidth"].to_int64() : 800;
  settingsData.windowHeight = root["windowHeight"].defined() ? root["windowHeight"].to_int64() : 600;
  settingsData.windowMinWidth = root["windowMinWidth"].defined() ? root["windowMinWidth"].to_int64() : 400;
  settingsData.windowMinHeight = root["windowMinHeight"].defined() ? root["windowMinHeight"].to_int64() : 300;
  settingsData.windowMaxWidth = root["windowMaxWidth"].defined() ? root["windowMaxWidth"].to_int64() : 3840;
  settingsData.windowMaxHeight = root["windowMaxHeight"].defined() ? root["windowMaxHeight"].to_int64() : 2160;
  settingsData.baseFontSize = root["baseFontSize"].defined() ? root["baseFontSize"].to_double() : 16.0f;
  settingsData.targetFps = root["targetFps"].defined() ? root["targetFps"].to_int64() : 60;
  settingsData.sfxVolume = root["sfxVolume"].defined() ? root["sfxVolume"].to_double() : 1.0f;
  settingsData.fontRegular = root["fontRegular"].defined() ? root["fontRegular"].to_string() : "";
  settingsData.fontBold = root["fontBold"].defined() ? root["fontBold"].to_string() : "";
  settingsData.fontItalic = root["fontItalic"].defined() ? root["fontItalic"].to_string() : "";
  settingsData.fontBoldItalic = root["fontBoldItalic"].defined() ? root["fontBoldItalic"].to_string() : "";
  settingsData.colorScheme = UI::Style::LoadTheme(root["colorScheme"]);
}

Data* Get() { return &settingsData; }

}  // namespace Settings
