#include <cstdarg>
#include <cstring>
#include <string>

// Vendor
#include "action.hpp"
#include "hjson.h"
#include "imhtml.hpp"
#include "io.hpp"
#include "log.hpp"
#include "raylib.h"
#include "rlImGui.h"

// Subsystems
#include "audio.hpp"
#include "elements.hpp"
#include "renderer.hpp"
#include "settings.hpp"
#include "state.hpp"
#include "texture_cache.hpp"
#include "ui/style.hpp"

//
// Request high performance mode to run on GPU if available
// instead of integrated graphics.
//
#ifdef _WIN32
typedef unsigned long DWORD;
extern "C" {
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

/**
 * @brief Parse argv for transport flags and return a TransportConfig.
 *
 * Supported flags:
 *   --socket <path>     Unix domain socket at <path>
 *   --pipe <path>       Named pipe at <path> (Unix domain socket on non-Windows)
 *
 * If no flag is present the default StdIO transport is used.
 *
 * @param argc Argument count from main().
 * @param argv Argument vector from main().
 * @return Configured IO::TransportConfig.
 */
static IO::TransportConfig ParseTransportFlags(int argc, char **argv) {
  IO::TransportConfig cfg;
  for (int i = 1; i < argc - 1; ++i) {
    if (std::strcmp(argv[i], "--socket") == 0) {
      cfg.mode = IO::TransportMode::UnixSocket;
      cfg.path = argv[i + 1];
      return cfg;
    }
    if (std::strcmp(argv[i], "--pipe") == 0) {
      cfg.mode = IO::TransportMode::NamedPipe;
      cfg.path = argv[i + 1];
      return cfg;
    }
  }
  return cfg;  // default: StdIO
}

int main(int argc, char **argv) {
  //
  // Parse CLI flags to select transport mode, then initialize the IO subsystem.
  //
  IO::TransportConfig transportConfig = ParseTransportFlags(argc, argv);
  IO::Init(transportConfig);

  //
  // Route raylib log messages through the protocol.
  //
  SetTraceLogCallback([](int logLevel, const char *text, va_list args) {
    char buffer[2048];
    vsnprintf(buffer, sizeof(buffer), text, args);
    Log::Print("Raylib", "%s", buffer);
  });

#ifdef GIT_TAG
  Log::Print("Main", "Starting stdui version %s", strlen(GIT_TAG) == 0 ? "unknown" : GIT_TAG);
#else
  Log::Print("Main", "Starting stdui version: unknown");
#endif

  //
  // Wait for settings to be loaded from stdin before initializing anything else.
  //
  auto settingsAction = Action::ExpectData(IO::MustReadValue(), "settings");
  if (!settingsAction) {
    Log::Print("Main", "Failed to load settings");
    IO::Shutdown();
    return 1;
  }

  Settings::Load(settingsAction.value());
  auto settings = Settings::Get();

  //
  // Init raylib.
  //
  if (settings->resizable) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  }

  InitWindow(settings->windowWidth, settings->windowHeight, settings->title.c_str());

  if (settings->windowMinWidth > 0 && settings->windowMinHeight > 0) {
    SetWindowMinSize(settings->windowMinWidth, settings->windowMinHeight);
  }

  if (settings->windowMaxWidth > 0 && settings->windowMaxHeight > 0) {
    SetWindowMaxSize(settings->windowMaxWidth, settings->windowMaxHeight);
  }

  InitAudioDevice();
  Log::Print("Main", "Raylib initialized");

  //
  // Configure raylib.
  //
  SetExitKey(KEY_NULL);
  SetTargetFPS(settings->targetFps);

  //
  // Register all ui-* custom elements.
  //
  Elements::RegisterAll();

  //
  // Setup ImGui styling.
  //
  rlImGuiBeginInitImGui();
  UI::Style::SetupStyle();
  rlImGuiEndInitImGui();

  //
  // Setup ImHTML image callbacks.
  //
  auto conf = ImHTML::GetConfig();
  conf->GetImageMeta = [](const char *url, const char *) {
    auto tex = TextureCache::GetPtr(url);
    return ImHTML::ImageMeta{tex->width, tex->height};
  };
  conf->GetImageTexture = [](const char *url, const char *) { return (ImTextureID)TextureCache::GetPtr(url); };
  conf->DefaultFont.Regular = UI::Style::GetFont(UI::Style::FontStyle::Regular);
  conf->DefaultFont.Bold = UI::Style::GetFont(UI::Style::FontStyle::Bold);
  conf->DefaultFont.Italic = UI::Style::GetFont(UI::Style::FontStyle::Italic);
  conf->DefaultFont.BoldItalic = UI::Style::GetFont(UI::Style::FontStyle::BoldItalic);

  //
  // Notify the controlling process that stdui is fully initialized and
  // ready to accept content.
  //
  Hjson::Value readyMsg;
  readyMsg["action"] = "ready";
  IO::WriteValue(readyMsg);

  //
  // Main game loop.
  //
  Log::Print("Main", "Mainloop starting...");
  while (!WindowShouldClose() && !CLOSE_REQUESTED) {
    Renderer::MainLoopIteration();
  }

  //
  // Cleanup.
  //
  Audio::Cleanup();
  rlImGuiShutdown();
  CloseWindow();

  //
  // Notify the controlling process that the window has been closed.
  //
  Hjson::Value windowClosed;
  windowClosed["action"] = "window-closed";
  IO::WriteValue(windowClosed);

  IO::Shutdown();

  return 0;
}
