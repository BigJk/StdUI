---
sidebar_position: 1
---

# Introduction

StdUI is a lightweight cross-platform UI engine that can be used with any programming language. It spawns as a child process and communicates with your application via stdin/stdout using newline-delimited JSON. This allows easy integration with any language that can start a process and read/write pipes.

:::warning
This project is experimental. Expect bugs, missing features, and breaking changes. The API is not stable and may change without deprecation.
:::

## How it works

```
Your App (Go, Python, anything)
        │  stdin  → JSON commands
        │  stdout ← JSON events
        ▼
   stdui binary (C++)
   ┌──────────────────────────────┐
   │  litehtml  — HTML/CSS layout │
   │  ImGui     — interactive     │
   │             widgets          │
   │  raylib    — window, audio   │
   └──────────────────────────────┘
```

StdUI uses a reduced subset of HTML/CSS (via [litehtml](https://github.com/litehtml/litehtml)) for layout and styling, and [ImGui](https://github.com/ocornut/imgui) for interactive widgets — with no browser involved.

## Features

- Cross-platform (Windows, macOS, Linux)
- Language agnostic (communicates via stdin/stdout)
- Supports a reduced subset of HTML/CSS for layout and styling
- Pane-based layout system
- Interactive widgets: buttons, text/number/password inputs, checkboxes, sliders, progress bars, color picker
- Native file/folder dialogs
- File drop support
- Sound playback
- Clipboard text access
- Notification toasts
- ~5MB binary size

## Quick start

### 1. Download the binary

Grab the latest release for your platform from [GitHub Releases](https://github.com/BigJk/StdUI/releases).

Alternatively, if you are using the Go SDK, `EnsureBinary` can download it automatically:

```go
if err := stdui.EnsureBinary("", ""); err != nil {
    panic(err)
}
```

### 2. Spawn and connect

Send a `settings` message immediately after spawning the process:

```json
{"action":"settings","data":{"title":"My App","windowWidth":800,"windowHeight":600}}
```

Wait for the `ready` event before sending any further commands:

```json
{"action":"ready"}
```

### 3. Render UI

Push HTML with `update-content`:

```json
{"action":"update-content","data":"<h1>Hello!</h1><ui-button text=\"Click me\" action=\"hello\"></ui-button>"}
```

### 4. React to events

Read stdout line by line and parse the JSON events:

```json
{"action":"button-clicked","data":{"action":"hello","text":"Click me","pane":"main"}}
```

Send a new `update-content` whenever your state changes. There is no incremental patching — replace the whole HTML string.
