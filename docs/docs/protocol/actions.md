---
sidebar_position: 1
---

# Core Actions

## `settings` — App → stdui

Configure the window and application on startup. Send this as the very first message after spawning the process; stdui blocks on startup until it is received.

```json
{
  "action": "settings",
  "data": {
    "title": "My App",
    "windowWidth": 1280,
    "windowHeight": 720,
    "baseFontSize": 14,
    "audioVolume": 0.5,
    "resizable": true
  }
}
```

All fields in `data` are optional. Only provided fields are applied; omitted fields keep their defaults.

| Field            | Type    | Default   | Description                                               |
| ---------------- | ------- | --------- | --------------------------------------------------------- |
| `title`          | string  | `"StdUI"` | Window title bar text                                     |
| `resizable`      | boolean | `true`    | Enable or disable window resizing                         |
| `windowWidth`    | number  | `800`     | Window width in pixels                                    |
| `windowHeight`   | number  | `600`     | Window height in pixels                                   |
| `windowMinWidth` | number  | `400`     | Minimum allowed window width in pixels                    |
| `windowMinHeight`| number  | `300`     | Minimum allowed window height in pixels                   |
| `windowMaxWidth` | number  | `3840`    | Maximum allowed window width in pixels                    |
| `windowMaxHeight`| number  | `2160`    | Maximum allowed window height in pixels                   |
| `baseFontSize`   | number  | `16.0`    | Base UI font size in pixels                               |
| `audioVolume`    | number  | `1.0`     | Audio volume level (0.0 to 1.0)                           |
| `fontRegular`    | string  | `""`      | Filename of the regular font file (empty = ImGui default) |
| `fontBold`       | string  | `""`      | Filename of the bold font file                            |
| `fontItalic`     | string  | `""`      | Filename of the italic font file                          |
| `fontBoldItalic` | string  | `""`      | Filename of the bold italic font file                     |
| `colorScheme`    | object  | —         | Color palette for the ImGui layer. See [Color Scheme](#color-scheme) |
| `targetFps`      | number  | `60`      | Target frame rate in frames per second                    |

### Color Scheme

The `colorScheme` object controls colors used by the ImGui layer (window backgrounds, buttons, borders, text, etc.). HTML content styled via CSS in your `update-content` strings is unaffected.

All values are hex strings in `#RRGGBB` or `#RRGGBBAA` format (e.g. `#0969da33` for 20% opacity). Omitted fields fall back to the built-in light theme defaults. `text` and `textMuted` are auto-derived from `windowBg` when omitted.

| Field                | Default             | Description                                                        |
| -------------------- | ------------------- | ------------------------------------------------------------------ |
| `windowBg`           | `#ffffff`           | Background of all pane windows                                     |
| `text`               | `#1f2328`           | Primary foreground text (auto-derived from `windowBg` if omitted)  |
| `textMuted`          | `#656d76`           | Secondary / muted text (auto-derived from `text` if omitted)       |
| `elementBg`          | `#f6f8fa`           | Default background of interactive elements                         |
| `elementHovered`     | `#eaeef2`           | Background of interactive elements on hover                        |
| `elementActive`      | `#d0d7de`           | Background of interactive elements while pressed                   |
| `titleBg`            | `#f6f8fa`           | Background of ImGui window title bars                              |
| `border`             | `#d0d7de`           | Element and window border color                                    |
| `primary`            | `#0969da`           | Accent color used for buttons and focus rings                      |
| `primaryTranslucent` | `#0969da` 80% alpha | Semi-transparent variant of `primary`                              |
| `primarySubtle`      | `#0969da` 10% alpha | Very lightly tinted background derived from `primary`              |
| `textSelectionBg`    | `#0969da` 20% alpha | Selected-text highlight color                                      |
| `warn`               | `#9a6700`           | Warning indicators                                                 |
| `danger`             | `#d1242f`           | Destructive actions and error indicators                           |
| `modalDim`           | `#000000` 30% alpha | Overlay color drawn behind modal dialogs                           |

**Example — dark theme:**

```json
{
  "action": "settings",
  "data": {
    "colorScheme": {
      "windowBg": "#0d1117",
      "text": "#e6edf3",
      "textMuted": "#7d8590",
      "elementBg": "#161b22",
      "elementHovered": "#1f2937",
      "elementActive": "#2d333b",
      "titleBg": "#161b22",
      "border": "#30363d",
      "primary": "#2f81f7",
      "warn": "#d29922",
      "danger": "#f85149"
    }
  }
}
```

---

## `update-content` — App → stdui

Sets the HTML content rendered in a pane. There is no incremental patching — each call replaces the entire content of the target pane.

**Plain string** (targets the default `"main"` pane):

```json
{ "action": "update-content", "data": "<h1>Hello</h1>" }
```

**Object form** (targets a named pane):

```json
{ "action": "update-content", "data": { "pane": "sidebar", "content": "<h1>Hello</h1>" } }
```

| Field     | Description                                                      |
| --------- | ---------------------------------------------------------------- |
| `pane`    | ID of the target pane, as declared in `set-pane-layout`          |
| `content` | HTML string to render                                            |

---

## `ready` — stdui → App

Emitted once the window and all subsystems are fully initialized. Do not send `update-content` until this is received.

```json
{ "action": "ready" }
```

---

## `close` — App → stdui

Instructs stdui to close the window and exit cleanly. stdui emits `window-closed` in response.

```json
{ "action": "close" }
```

---

## `window-closed` — stdui → App

Emitted when the user closes the window (or after a `close` action). The controlling app should use this as its shutdown signal.

```json
{ "action": "window-closed" }
```

---

## `log` — stdui → App

Emitted for all internal log messages, including output from raylib.

```json
{ "action": "log", "data": { "namespace": "Main", "message": "Raylib initialized" } }
```

| Field       | Description                                                              |
| ----------- | ------------------------------------------------------------------------ |
| `namespace` | Category or subsystem that produced the message (e.g. `"Main"`, `"Font"`) |
| `message`   | The log message text                                                     |
