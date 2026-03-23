# Protocol

The stdui framework communicates with the application via stdin/stdout using line-delimited JSON.

## Message Format

All messages are single lines of JSON, terminated by a newline character. Each message is a JSON object that may contain the following fields:

```json
{
  "action": "action_type",
  "data": {
    /* optional data payload */
  }
}
```

### Fields

- **`action`** _(string, required)_ — The type of action being sent.
- **`data`** _(object, optional)_ — Action-specific data payload.

## Actions

Action names use kebab-case (e.g. `update-content`, `window-closed`).

### `settings`

Configure application settings. Sent by the application to the stdui framework to update window properties, font settings, color scheme, and other UI-related configurations.

**Request:**

```json
{
  "action": "settings",
  "data": {
    "key": "value"
  }
}
```

**Fields (in `data`):**

All fields are optional and only provided fields will be updated.

- **`title`** _(string)_ — Window title bar text. Defaults to `"StdUI"`.
- **`resizable`** _(boolean)_ — Enable or disable window resizing. Defaults to `true`.
- **`windowWidth`** _(number)_ — Window width in pixels. Defaults to `800`.
- **`windowHeight`** _(number)_ — Window height in pixels. Defaults to `600`.
- **`windowMinWidth`** _(number)_ — Minimum allowed window width in pixels. Defaults to `400`.
- **`windowMinHeight`** _(number)_ — Minimum allowed window height in pixels. Defaults to `300`.
- **`windowMaxWidth`** _(number)_ — Maximum allowed window width in pixels. Defaults to `3840`.
- **`windowMaxHeight`** _(number)_ — Maximum allowed window height in pixels. Defaults to `2160`.
- **`baseFontSize`** _(number)_ — Base UI font size in pixels. Defaults to `16.0`.
- **`sfxVolume`** _(number)_ — Sound effects volume level (0.0 to 1.0). Defaults to `1.0`.
- **`fontRegular`** _(string)_ — Filename of the regular font file. Defaults to `""` (use imgui default).
- **`fontBold`** _(string)_ — Filename of the bold font file. Defaults to `""` (use imgui default).
- **`fontItalic`** _(string)_ — Filename of the italic font file. Defaults to `""` (use imgui default).
- **`fontBoldItalic`** _(string)_ — Filename of the bold italic font file. Defaults to `""` (use imgui default).
- **`colorScheme`** _(object)_ — Color palette for the UI. All fields are hex color strings in `#RRGGBB` or `#RRGGBBAA` format. Omitted fields fall back to the built-in light theme defaults. See [Color Scheme](#color-scheme) below.
- **`targetFps`** _(number)_ — Target frame rate in frames per second. Defaults to `60`.

**Example:**

```json
{
  "action": "settings",
  "data": {
    "title": "My App",
    "windowWidth": 1280,
    "windowHeight": 720,
    "baseFontSize": 14,
    "sfxVolume": 0.5,
    "resizable": true
  }
}
```

#### Color Scheme

The `colorScheme` object controls every color used by the ImGui layer (window backgrounds, buttons, borders, text, etc.). HTML content rendered by litehtml is styled via normal CSS in your HTML strings and is unaffected by this setting.

All values are hex strings. Two formats are accepted:

- **`#RRGGBB`** — fully opaque
- **`#RRGGBBAA`** — with explicit alpha (e.g. `#0969da33` for 20% opacity)

If a field is omitted it falls back to the built-in light theme default shown in the table below. `text` and `textMuted` are additionally auto-derived from `windowBg` when omitted — a dark background produces light text automatically.

| Field                | Default             | Description                                                       |
| -------------------- | ------------------- | ----------------------------------------------------------------- |
| `windowBg`           | `#ffffff`           | Background of all pane windows                                    |
| `text`               | `#1f2328`           | Primary foreground text (auto-derived from `windowBg` if omitted) |
| `textMuted`          | `#656d76`           | Secondary / muted text (auto-derived from `text` if omitted)      |
| `elementBg`          | `#f6f8fa`           | Default background of interactive elements                        |
| `elementHovered`     | `#eaeef2`           | Background of interactive elements on hover                       |
| `elementActive`      | `#d0d7de`           | Background of interactive elements while pressed                  |
| `titleBg`            | `#f6f8fa`           | Background of ImGui window title bars                             |
| `border`             | `#d0d7de`           | Element and window border color                                   |
| `primary`            | `#0969da`           | Accent / brand color used for buttons and focus rings             |
| `primaryTranslucent` | `#0969da` 80% alpha | Semi-transparent variant of `primary`                             |
| `primarySubtle`      | `#0969da` 10% alpha | Very lightly tinted background derived from `primary`             |
| `textSelectionBg`    | `#0969da` 20% alpha | Selected-text highlight color                                     |
| `warn`               | `#9a6700`           | Warning indicators                                                |
| `danger`             | `#d1242f`           | Destructive actions and error indicators                          |
| `modalDim`           | `#000000` 30% alpha | Overlay color drawn behind modal dialogs                          |

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

### `update-content`

Sets the HTML content rendered in the window. Two forms are accepted:

**Plain string (backwards-compatible)** — targets the default `"main"` pane:

**Direction:** controlling app → stdui

```json
{ "action": "update-content", "data": "<h1>Hello</h1>" }
```

**Object form** — targets a named pane in the current layout:

```json
{ "action": "update-content", "data": { "pane": "sidebar", "content": "<h1>Hello</h1>" } }
```

- **`pane`** _(string)_ — ID of the target pane, as declared in `set-pane-layout`.
- **`content`** _(string)_ — HTML content to render in that pane.

### `window-closed`

Emitted by stdui when the user closes the window. The controlling app should use this to shut down.

**Direction:** stdui → controlling app

```json
{ "action": "window-closed" }
```

### `ready`

Emitted by stdui once the window and all subsystems are fully initialized. The controlling app should wait for this before sending the first `update-content` message to avoid a race condition where content arrives before the renderer is ready.

**Direction:** stdui → controlling app

```json
{ "action": "ready" }
```

### `close`

Instructs stdui to close the window and exit cleanly. Equivalent to the user pressing the OS close button. stdui will emit `window-closed` in response.

**Direction:** controlling app → stdui

```json
{ "action": "close" }
```

### `log`

Emitted by stdui for all internal log messages, including raylib output. The controlling app can display or store these as appropriate.

**Direction:** stdui → controlling app

```json
{ "action": "log", "data": { "namespace": "Main", "message": "Raylib initialized" } }
```

**Fields (in `data`):**

- **`namespace`** _(string)_ — Category or subsystem that produced the message (e.g. `"Main"`, `"Raylib"`, `"Font"`).
- **`message`** _(string)_ — The log message text.

---

## Interactive Elements

Interactive elements are custom HTML tags rendered by ImGui. Each requires an `id` attribute used to track state and identify events. State is seeded from the `value`/`checked` attribute on first render and persisted internally until changed by the user or a `set-value` action.

All interactive element tags use the `ui-` prefix (e.g. `<ui-button>`, `<ui-input>`) to avoid conflicts with litehtml's built-in CSS rules for standard HTML tags.

### `<ui-button>`

```html
<ui-button text="Click Me" action="my-action" tooltip="Optional"></ui-button>
```

| Attribute | Required | Description                                   |
| --------- | -------- | --------------------------------------------- |
| `text`    | yes      | Button label                                  |
| `action`  | yes      | Identifier sent back in `button-clicked` data |
| `tooltip` | no       | Hover tooltip text                            |

Emits `button-clicked` with all attributes as `data`.

### `<ui-input>`

```html
<ui-input id="username" type="text" placeholder="Enter name" value=""></ui-input>
<ui-input id="age" type="number" min="0" max="120" step="0.5" value="18"></ui-input>
<ui-input id="port" type="int" min="1" max="65535" step="1" value="8080"></ui-input>
<ui-input id="secret" type="password" placeholder="Enter password"></ui-input>
<ui-input id="search" type="text" live="true" placeholder="Search…"></ui-input>
```

| Attribute     | Required | Description                                                                                                              |
| ------------- | -------- | ------------------------------------------------------------------------------------------------------------------------ |
| `id`          | yes      | Element identifier                                                                                                       |
| `type`        | no       | `text` (default), `number`, `int`, or `password`                                                                         |
| `placeholder` | no       | Ghost text for `text` and `password` inputs                                                                              |
| `value`       | no       | Initial value                                                                                                            |
| `min` / `max` | no       | Clamp range for `number` and `int` inputs                                                                                |
| `step`        | no       | Arrow-button increment for `number` and `int` inputs. Defaults to `1` for `int`; omit or `0` to hide arrows for `number` |
| `live`        | no       | `"true"` to emit `input-changed` on every keystroke instead of only on Enter or focus loss. Defaults to `"false"`        |

Emits `input-changed` on Enter or focus loss (default), or on every keystroke when `live="true"`.

### `<ui-select>`

```html
<ui-select id="theme" options="Light|Dark|System" value="Light"></ui-select>
```

| Attribute | Required | Description                                                                               |
| --------- | -------- | ----------------------------------------------------------------------------------------- |
| `id`      | yes      | Element identifier                                                                        |
| `options` | yes      | Pipe-separated (`\|`) list of options. Use `\\\|` to include a literal `\|` in an option. |
| `value`   | no       | Initially selected option (defaults to first)                                             |

Emits `input-changed` immediately on selection change.

### `<ui-slider>`

```html
<ui-slider id="volume" min="0" max="1" value="0.5"></ui-slider>
<ui-slider id="speed" min="0" max="200" value="100" type="int" format="%d km/h"></ui-slider>
```

| Attribute | Required | Description                                                                                             |
| --------- | -------- | ------------------------------------------------------------------------------------------------------- |
| `id`      | yes      | Element identifier                                                                                      |
| `min`     | no       | Minimum value (default `0`)                                                                             |
| `max`     | no       | Maximum value (default `1`)                                                                             |
| `value`   | no       | Initial value                                                                                           |
| `type`    | no       | `float` (default) or `int` — selects `SliderFloat` vs `SliderInt` under the hood                        |
| `format`  | no       | `printf`-style format string for the displayed label. Defaults to `"%.3f"` for float and `"%d"` for int |

Emits `input-changed` on mouse release. The emitted value is an integer string when `type="int"`, a float string otherwise.

### `<ui-checkbox>`

```html
<ui-checkbox id="notifications" label="Enable notifications" checked="true"></ui-checkbox>
```

| Attribute | Required | Description                                              |
| --------- | -------- | -------------------------------------------------------- |
| `id`      | yes      | Element identifier                                       |
| `label`   | no       | Label shown next to the checkbox                         |
| `checked` | no       | Initial state: `"true"` or `"false"` (default `"false"`) |

Emits `input-changed` immediately on toggle with value `"true"` or `"false"`.

### `<ui-textarea>`

```html
<ui-textarea id="notes" placeholder="Enter notes..." rows="4" value=""></ui-textarea>
<ui-textarea id="live-notes" live="true" placeholder="Live notes..."></ui-textarea>
```

| Attribute     | Required | Description                                                                                              |
| ------------- | -------- | -------------------------------------------------------------------------------------------------------- |
| `id`          | yes      | Element identifier                                                                                       |
| `placeholder` | no       | Ghost text shown when empty                                                                              |
| `value`       | no       | Initial content                                                                                          |
| `rows`        | no       | Visual row hint (default `4`); override actual height via CSS `height` in your content's `<style>` block |
| `live`        | no       | `"true"` to emit `input-changed` on every keystroke instead of only on focus loss. Defaults to `"false"` |

Emits `input-changed` on focus loss (default), or on every keystroke when `live="true"`. Supports **Tab** for indentation. **Enter** inserts a newline.

### `<ui-progress>`

```html
<ui-progress id="upload" value="0.4" min="0" max="1" overlay="40%"></ui-progress>
```

Read-only progress bar. The fill fraction is computed as `(value - min) / (max - min)`.
Update the displayed value at runtime via `set-value`.

| Attribute | Required | Description                                                    |
| --------- | -------- | -------------------------------------------------------------- |
| `id`      | yes      | Element identifier; used with `set-value` to update the bar    |
| `value`   | no       | Current value (default `0`)                                    |
| `min`     | no       | Minimum value (default `0`)                                    |
| `max`     | no       | Maximum value (default `1`)                                    |
| `overlay` | no       | Text drawn on top of the bar (e.g. `"40%"`); omit for no label |

Does not emit any events.

### `<ui-color>`

```html
<ui-color id="accent" value="#0969da"></ui-color> <ui-color id="overlay" value="#0969da80" alpha="true"></ui-color>
```

Renders an inline color swatch with a hex input field (ImGui `ColorEdit3` / `ColorEdit4`). Clicking the swatch opens an expanded color picker popup.

| Attribute | Required | Description                                                                                              |
| --------- | -------- | -------------------------------------------------------------------------------------------------------- |
| `id`      | yes      | Element identifier                                                                                       |
| `value`   | no       | Initial color as a hex string. `#RRGGBB` when `alpha="false"` (default), `#RRGGBBAA` when `alpha="true"` |
| `alpha`   | no       | `"true"` to enable the alpha channel. Defaults to `"false"`                                              |

Emits `input-changed` immediately on every color change. The value is always a lowercase hex string: `#rrggbb` or `#rrggbbaa`.

### `<ui-file-select>`

```html
<ui-file-select id="input-file" label="Browse..." filter="png,jpg" value=""></ui-file-select>
```

Renders a read-only path display next to a button. Clicking the button opens a native OS file-open dialog synchronously on the main thread. When the user picks a file, the path is stored in element state and an `input-changed` event is emitted. If the user cancels, no event is emitted.

| Attribute | Required | Description                                                                      |
| --------- | -------- | -------------------------------------------------------------------------------- |
| `id`      | yes      | Element identifier                                                               |
| `label`   | no       | Button text (default `"Browse..."`)                                              |
| `filter`  | no       | Comma-separated file extensions to filter, e.g. `"png,jpg"` (default: all files) |
| `value`   | no       | Initial path shown in the display                                                |

Emits `input-changed` with the selected absolute file path as the value.

### `<ui-folder-select>`

```html
<ui-folder-select id="output-dir" label="Browse..." value=""></ui-folder-select>
```

Same as `<ui-file-select>` but opens a native folder-picker dialog instead.

| Attribute | Required | Description                         |
| --------- | -------- | ----------------------------------- |
| `id`      | yes      | Element identifier                  |
| `label`   | no       | Button text (default `"Browse..."`) |
| `value`   | no       | Initial path shown in the display   |

Emits `input-changed` with the selected absolute folder path as the value.

### `<ui-file-save>`

```html
<ui-file-save id="output-file" label="Save..." filter="png,jpg" value=""></ui-file-save>
```

Renders a read-only path display next to a button. Clicking the button opens a native OS save-file dialog synchronously on the main thread. When the user confirms, the chosen path is stored in element state and an `input-changed` event is emitted. If the user cancels, no event is emitted.

| Attribute | Required | Description                                                                      |
| --------- | -------- | -------------------------------------------------------------------------------- |
| `id`      | yes      | Element identifier                                                               |
| `label`   | no       | Button text (default `"Save..."`)                                                |
| `filter`  | no       | Comma-separated file extensions to filter, e.g. `"png,jpg"` (default: all files) |
| `value`   | no       | Initial path shown in the display                                                |

Emits `input-changed` with the chosen absolute file path as the value.

---

## Element Actions

### `input-changed` _(stdui → controlling app)_

Emitted when the user commits a change to any interactive element.

```json
{ "action": "input-changed", "data": { "id": "volume", "value": "0.75", "pane": "main" } }
```

- **`id`** — The element's `id` attribute.
- **`value`** — The new value as a string. For checkboxes: `"true"` or `"false"`.
- **`pane`** — The id of the pane the element belongs to.

### `button-clicked` _(stdui → controlling app)_

Emitted when a button is clicked. `data` contains all HTML attributes of the button, plus a `pane` field.

```json
{ "action": "button-clicked", "data": { "action": "save", "text": "Save", "pane": "main" } }
```

- **`pane`** — The id of the pane the button belongs to.
- All other fields are the HTML attributes of the `<ui-button>` element.

### `set-value` _(controlling app → stdui)_

Programmatically set the value of an element by id. Useful for pre-populating fields.

```json
{ "action": "set-value", "data": { "id": "username", "value": "Alice" } }
```

### `get-value` _(controlling app → stdui)_

Request the current value of an element by id. stdui responds with a `value-result` action.

The `data` field may be either a plain string (the id) or an object with an `id` field:

```json
{ "action": "get-value", "data": "username" }
{ "action": "get-value", "data": { "id": "username" } }
```

stdui replies on stdout:

```json
{ "action": "value-result", "data": { "id": "username", "value": "Alice" } }
```

- **`id`** — The element's `id` attribute.
- **`value`** — Current value as a string, or `""` if the id has never been rendered or set.

### `set-title` _(controlling app → stdui)_

Updates the window title bar text at runtime.

```json
{ "action": "set-title", "data": "My App — unsaved changes" }
```

- **`data`** — The new title string.

### `set-window-icon` _(controlling app → stdui)_

Sets the window and taskbar icon from a single image file. The file must be in
a format supported by raylib (PNG, BMP, TGA, JPG, GIF). For best results use a
square RGBA image; 64×64 or 256×256 are common choices.

The `data` field may be a plain path string or an object with a `path` field:

```json
{ "action": "set-window-icon", "data": "./assets/icon.png" }
{ "action": "set-window-icon", "data": { "path": "./assets/icon.png" } }
```

- **`data`** — Path to the image file (absolute or relative to the stdui working directory).

### `set-window-icons` _(controlling app → stdui)_

Sets the window and taskbar icon from multiple image files, supplying several
resolutions so the OS can pick the most appropriate one. The files must be in a
format supported by raylib (PNG, BMP, TGA, JPG, GIF).

```json
{ "action": "set-window-icons", "data": ["./assets/icon16.png", "./assets/icon32.png", "./assets/icon256.png"] }
```

- **`data`** — Array of image file paths (absolute or relative to the stdui working directory). Paths that fail to load are skipped with a log warning.

### `file-dropped` _(stdui → controlling app)_

Emitted once per file when the user drags and drops files onto the stdui window.
Each dropped file produces a separate `file-dropped` message.

```json
{ "action": "file-dropped", "data": { "path": "/home/user/photo.png" } }
```

- **`path`** — Absolute path to the dropped file.

### `minimize` _(controlling app → stdui)_

Minimizes the window to the taskbar / dock.

```json
{ "action": "minimize" }
```

### `maximize` _(controlling app → stdui)_

Maximizes the window to fill the screen.

```json
{ "action": "maximize" }
```

### `set-position` _(controlling app → stdui)_

Moves the window to the given screen coordinates.

```json
{ "action": "set-position", "data": { "x": 100, "y": 200 } }
```

- **`x`** — Horizontal screen position in pixels.
- **`y`** — Vertical screen position in pixels.

### `play-sound` _(controlling app → stdui)_

Plays a sound file through stdui's audio system (respects the `sfxVolume` setting).
The `data` field is the path to the sound file — absolute or relative to the stdui working directory.

```json
{ "action": "play-sound", "data": "./assets/sounds/click.ogg" }
```

### `set-volume` _(controlling app → stdui)_

Updates the sound effects volume at runtime. The value is a float in the range
`[0.0, 1.0]`; values outside this range are clamped. Affects all subsequent
`play-sound` calls — already-playing sounds are not retroactively changed.

The `data` field may be a plain number or an object with a `volume` field:

```json
{ "action": "set-volume", "data": 0.5 }
{ "action": "set-volume", "data": { "volume": 0.5 } }
```

- **`data`** — New volume level (0.0 = silent, 1.0 = full volume).

### `set-fps` _(controlling app → stdui)_

Updates the target frame rate at runtime. Useful for throttling the render loop when the window is idle or to boost it for animations.

The `data` field may be either a plain number (the new FPS) or an object with an `fps` field.
Values of `0` or below are ignored.

```json
{ "action": "set-fps", "data": 30 }
{ "action": "set-fps", "data": { "fps": 120 } }
```

- **`data`** — Target frames per second (positive integer).

### `confirm` _(controlling app → stdui)_

Opens a modal dialog with a question and two buttons. The modal blocks interaction with the underlying window until the user responds.

```json
{
  "action": "confirm",
  "data": { "id": "delete-file", "question": "Delete this file?", "title": "Confirm", "ok-text": "Delete", "cancel-text": "Cancel" }
}
```

- **`id`** _(required)_ — Opaque identifier echoed back in the `confirm-result` response.
- **`question`** _(required)_ — The message shown inside the dialog.
- **`title`** _(optional)_ — Dialog heading. Defaults to `"Confirm"`.
- **`ok-text`** _(optional)_ — Label for the confirmation button. Defaults to `"OK"`.
- **`cancel-text`** _(optional)_ — Label for the cancellation button. Defaults to `"Cancel"`.

### `confirm-result` _(stdui → controlling app)_

Emitted once the user dismisses a `confirm` dialog by clicking either button.

```json
{ "action": "confirm-result", "data": { "id": "delete-file", "result": true } }
```

- **`id`** — The `id` from the originating `confirm` action.
- **`result`** — `true` if the user clicked the OK button, `false` if they clicked Cancel.

### `url-clicked` _(stdui → controlling app)_

Emitted when the user clicks a hyperlink (`<a href="...">`) inside any pane.

```json
{ "action": "url-clicked", "data": { "url": "https://example.com", "pane": "main" } }
```

- **`url`** — The value of the `href` attribute.
- **`pane`** — The id of the pane in which the link was clicked.

### `toast` _(controlling app → stdui)_

Queues a short-lived notification window rendered at the top-center of the
display. Toasts stack downward (newest at the bottom of the stack) and are
removed automatically once their TTL expires. The user may also double-click
inside a toast to dismiss it early.

```json
{
  "action": "toast",
  "data": {
    "content": "<b>File saved</b>",
    "width": 300,
    "height": 60,
    "ttl": 3.0
  }
}
```

**Fields (in `data`):**

| Field     | Required | Default | Description                                                   |
| --------- | -------- | ------- | ------------------------------------------------------------- |
| `content` | yes      | —       | HTML string rendered inside the toast window via litehtml     |
| `width`   | no       | `300`   | Toast window width in pixels                                  |
| `height`  | no       | `0`     | Toast window height in pixels (0 = grow to fit content)       |
| `ttl`     | no       | `3.0`   | Time-to-live in seconds; toast is removed after this duration |

The toast window has no decorations and a fully transparent background — all
visual styling (background color, border, padding, rounded corners, etc.) must
be applied inside the `content` HTML via inline CSS or a `<style>` block.

**Example — styled card toast:**

```json
{
  "action": "toast",
  "data": {
    "content": "<div style=\"background:#1f2328;color:#e6edf3;border-radius:8px;padding:12px 16px;font-size:1rem;\"><b>Saved</b> — changes written to disk.</div>",
    "width": 320,
    "height": 56,
    "ttl": 4.0
  }
}
```

### `set-clipboard-text` _(controlling app → stdui)_

Writes text to the system clipboard via raylib's `SetClipboardText`.

The `data` field may be a plain string or an object with a `text` field:

```json
{ "action": "set-clipboard-text", "data": "Hello, clipboard!" }
{ "action": "set-clipboard-text", "data": { "text": "Hello, clipboard!" } }
```

- **`data`** — The string to write to the clipboard.

### `get-clipboard-text` _(controlling app → stdui)_

Requests the current system clipboard text via raylib's `GetClipboardText`.
stdui responds asynchronously with a `clipboard-text-result` event.

```json
{ "action": "get-clipboard-text" }
```

stdui replies on stdout:

```json
{ "action": "clipboard-text-result", "data": { "text": "Hello, clipboard!" } }
```

- **`text`** — Current clipboard contents as a string, or `""` if the clipboard is empty or unavailable.

### `clipboard-text-result` _(stdui → controlling app)_

Emitted in response to a `get-clipboard-text` request.

```json
{ "action": "clipboard-text-result", "data": { "text": "Hello, clipboard!" } }
```

- **`text`** — The current clipboard contents.

---

## Keybinds

### `set-keybinds` _(controlling app → stdui)_

Registers a set of keyboard shortcuts. Calling this action replaces any previously registered shortcuts. When a registered shortcut is triggered, stdui emits a `key-pressed` event.

```json
{
  "action": "set-keybinds",
  "data": [
    { "id": "save",      "key": "s",      "ctrl": true },
    { "id": "find",      "key": "f",      "ctrl": true },
    { "id": "quit",      "key": "q",      "ctrl": true },
    { "id": "help",      "key": "f1" },
    { "id": "fullscreen","key": "f11" },
    { "id": "new-item",  "key": "n",      "ctrl": true, "shift": true }
  ]
}
```

**Fields (per entry in `data` array):**

| Field   | Required | Description                                                                        |
| ------- | -------- | ---------------------------------------------------------------------------------- |
| `id`    | yes      | Application-defined identifier echoed back in `key-pressed`.                       |
| `key`   | yes      | Primary key name (case-insensitive). See [Key Names](#key-names) below.            |
| `ctrl`  | no       | Require Ctrl (left or right) to be held. Defaults to `false`.                      |
| `shift` | no       | Require Shift (left or right) to be held. Defaults to `false`.                     |
| `alt`   | no       | Require Alt (left or right) to be held. Defaults to `false`.                       |
| `meta`  | no       | Require Super / Cmd (left or right) to be held. Defaults to `false`.               |

To clear all keybinds, send an empty array: `{ "action": "set-keybinds", "data": [] }`.

#### Key Names

Single characters are accepted as-is: `"a"`–`"z"`, `"0"`–`"9"`, `"/"`, `","`, `"."`, `"-"`, `"="`, `";"`, `"'"`, `"["`, `"]"`, `"\\"`, `` "`" ``.

Named keys:

| Name           | Key              |
| -------------- | ---------------- |
| `space`        | Space bar        |
| `enter` / `return` | Enter        |
| `tab`          | Tab              |
| `backspace`    | Backspace        |
| `delete` / `del` | Delete         |
| `insert`       | Insert           |
| `escape` / `esc` | Escape         |
| `left`         | Arrow left       |
| `right`        | Arrow right      |
| `up`           | Arrow up         |
| `down`         | Arrow down       |
| `home`         | Home             |
| `end`          | End              |
| `pageup`       | Page Up          |
| `pagedown`     | Page Down        |
| `capslock`     | Caps Lock        |
| `scrolllock`   | Scroll Lock      |
| `numlock`      | Num Lock         |
| `printscreen`  | Print Screen     |
| `pause`        | Pause            |
| `f1`–`f12`     | Function keys    |

### `key-pressed` _(stdui → controlling app)_

Emitted when a registered keyboard shortcut is activated.

```json
{ "action": "key-pressed", "data": { "id": "save" } }
```

- **`id`** — The application-defined identifier from the matching `set-keybinds` entry.

---

## Scroll Control

### `scroll-to` _(controlling app → stdui)_

Programmatically scrolls a pane to a given vertical position. The scroll is
applied on the next rendered frame.

```json
{ "action": "scroll-to", "data": { "pane": "history", "position": "bottom" } }
```

**Fields (in `data`):**

| Field      | Type             | Description                                                                                                                                                           |
| ---------- | ---------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `pane`     | string, required | ID of the target pane.                                                                                                                                                |
| `position` | number or string | Vertical scroll target. `"bottom"` scrolls to the end of content; a number is an absolute pixel offset from the top. Omitting `position` (or `0`) scrolls to the top. |

**Examples:**

```json
{ "action": "scroll-to", "data": { "pane": "chat", "position": "bottom" } }
{ "action": "scroll-to", "data": { "pane": "log",  "position": 0 } }
{ "action": "scroll-to", "data": { "pane": "list", "position": 320 } }
```

---

## Pane Layout

### `set-pane-layout` _(controlling app → stdui)_

Replaces the current window layout with a new pane tree. The tree is a recursive
structure of pane leaves and split nodes. Leaf panes are independently renderable
areas; split nodes divide their allocated space among their children according to
relative size weights.

```json
{
  "action": "set-pane-layout",
  "data": {
    "type": "split",
    "direction": "horizontal",
    "children": [
      { "type": "pane", "id": "sidebar", "size": 1 },
      {
        "type": "split",
        "direction": "vertical",
        "size": 3,
        "children": [
          { "type": "pane", "id": "top" },
          { "type": "pane", "id": "bottom" }
        ]
      }
    ]
  }
}
```

**Node fields:**

| Field       | Required for | Description                                                                                                                    |
| ----------- | ------------ | ------------------------------------------------------------------------------------------------------------------------------ |
| `type`      | all nodes    | `"pane"` for a leaf or `"split"` for an interior divider.                                                                      |
| `id`        | pane nodes   | Unique identifier used as the target for `update-content`.                                                                     |
| `direction` | split nodes  | `"horizontal"` (left-to-right) or `"vertical"` (top-to-bottom).                                                                |
| `children`  | split nodes  | Array of child nodes.                                                                                                          |
| `size`      | all nodes    | Size hint for the parent split. **<= 10**: relative weight (higher = more space). **> 10**: fixed pixel size. Defaults to `1`. |

When `set-pane-layout` is not called, the layout defaults to a single pane with
id `"main"`. The plain-string form of `update-content` always targets `"main"`,
making it fully backwards-compatible with single-pane usage.

#### Size semantics

- **`size <= 10`** — relative weight. After all fixed-pixel children are
  allocated, the remaining space is divided proportionally among
  relative-weight children. A child with `size: 2` gets twice the flexible
  space of a child with `size: 1`.
- **`size > 10`** — fixed pixel size. The child is allocated exactly that many
  pixels in the split axis regardless of available space.

Mixed fixed and relative children are fully supported within the same split.
