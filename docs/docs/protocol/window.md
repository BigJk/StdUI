---
sidebar_position: 4
---

# Window Control

Actions for controlling the window at runtime.

## `set-title` — App → stdui

Updates the window title bar text.

```json
{ "action": "set-title", "data": "My App — unsaved changes" }
```

---

## `set-window-icon` — App → stdui

Sets the window and taskbar icon from a single image file (PNG, BMP, TGA, JPG, GIF). For best results use a square RGBA image (64×64 or 256×256).

The `data` field may be a plain path string or an object with a `path` field:

```json
{ "action": "set-window-icon", "data": "./assets/icon.png" }
{ "action": "set-window-icon", "data": { "path": "./assets/icon.png" } }
```

---

## `set-window-icons` — App → stdui

Sets the window icon from multiple image files so the OS can pick the most appropriate resolution. Paths that fail to load are skipped with a log warning.

```json
{ "action": "set-window-icons", "data": ["./assets/icon16.png", "./assets/icon32.png", "./assets/icon256.png"] }
```

---

## `minimize` — App → stdui

Minimizes the window to the taskbar / dock.

```json
{ "action": "minimize" }
```

---

## `maximize` — App → stdui

Maximizes the window to fill the screen.

```json
{ "action": "maximize" }
```

---

## `set-position` — App → stdui

Moves the window to the given screen coordinates.

```json
{ "action": "set-position", "data": { "x": 100, "y": 200 } }
```

| Field | Description                              |
| ----- | ---------------------------------------- |
| `x`   | Horizontal screen position in pixels     |
| `y`   | Vertical screen position in pixels       |

---

## `set-fps` — App → stdui

Updates the target frame rate at runtime. Useful for throttling when idle or boosting for animations. Values of `0` or below are ignored.

The `data` field may be a plain number or an object with an `fps` field:

```json
{ "action": "set-fps", "data": 30 }
{ "action": "set-fps", "data": { "fps": 120 } }
```

---

## `toast` — App → stdui

Queues a short-lived notification window at the top-center of the display. Toasts stack downward and are removed once their TTL expires. The user may double-click a toast to dismiss it early.

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

| Field     | Required | Default | Description                                                    |
| --------- | -------- | ------- | -------------------------------------------------------------- |
| `content` | yes      | —       | HTML string rendered inside the toast via litehtml             |
| `width`   | no       | `300`   | Toast window width in pixels                                   |
| `height`  | no       | `0`     | Toast window height (0 = grow to fit content)                  |
| `ttl`     | no       | `3.0`   | Time-to-live in seconds                                        |

The toast window has no decorations and a fully transparent background — all visual styling must be applied inside `content` via inline CSS or a `<style>` block.

**Example — styled card:**

```json
{
  "action": "toast",
  "data": {
    "content": "<div style=\"background:#1f2328;color:#e6edf3;border-radius:8px;padding:12px 16px;\"><b>Saved</b> — changes written to disk.</div>",
    "width": 320,
    "height": 56,
    "ttl": 4.0
  }
}
```

---

## `confirm` — App → stdui

Opens a modal dialog with a question and two buttons. Blocks interaction with the window until the user responds.

```json
{
  "action": "confirm",
  "data": { "id": "delete-file", "question": "Delete this file?", "title": "Confirm", "ok-text": "Delete", "cancel-text": "Cancel" }
}
```

| Field         | Required | Default      | Description                                          |
| ------------- | -------- | ------------ | ---------------------------------------------------- |
| `id`          | yes      | —            | Opaque identifier echoed back in `confirm-result`    |
| `question`    | yes      | —            | The message shown inside the dialog                  |
| `title`       | no       | `"Confirm"`  | Dialog heading                                       |
| `ok-text`     | no       | `"OK"`       | Label for the confirmation button                    |
| `cancel-text` | no       | `"Cancel"`   | Label for the cancellation button                    |

---

## `confirm-result` — stdui → App

Emitted once the user dismisses a `confirm` dialog.

```json
{ "action": "confirm-result", "data": { "id": "delete-file", "result": true } }
```

| Field    | Description                                                         |
| -------- | ------------------------------------------------------------------- |
| `id`     | The `id` from the originating `confirm` action                      |
| `result` | `true` if the user clicked OK, `false` if they clicked Cancel       |

---

## `url-clicked` — stdui → App

Emitted when the user clicks a hyperlink (`<a href="...">`) inside any pane.

```json
{ "action": "url-clicked", "data": { "url": "https://example.com", "pane": "main" } }
```

| Field  | Description                                      |
| ------ | ------------------------------------------------ |
| `url`  | The value of the `href` attribute                |
| `pane` | The id of the pane in which the link was clicked |

---

## `play-sound` — App → stdui

Plays a sound file through stdui's audio system. Respects the `audioVolume` setting.

```json
{ "action": "play-sound", "data": "./assets/sounds/click.ogg" }
```

---

## `set-volume` — App → stdui

Updates the audio volume at runtime (0.0–1.0). Affects all subsequent `play-sound` calls; already-playing sounds are not retroactively changed.

```json
{ "action": "set-volume", "data": 0.5 }
{ "action": "set-volume", "data": { "volume": 0.5 } }
```

---

## `set-clipboard-text` — App → stdui

Writes text to the system clipboard.

```json
{ "action": "set-clipboard-text", "data": "Hello, clipboard!" }
{ "action": "set-clipboard-text", "data": { "text": "Hello, clipboard!" } }
```

---

## `get-clipboard-text` — App → stdui

Requests the current clipboard text. stdui responds with a `clipboard-text-result` event.

```json
{ "action": "get-clipboard-text" }
```

---

## `clipboard-text-result` — stdui → App

Emitted in response to `get-clipboard-text`.

```json
{ "action": "clipboard-text-result", "data": { "text": "Hello, clipboard!" } }
```

| Field  | Description                                                        |
| ------ | ------------------------------------------------------------------ |
| `text` | Current clipboard contents, or `""` if empty or unavailable        |

---

## `file-dropped` — stdui → App

Emitted once per file when the user drags and drops files onto the window. Each dropped file produces a separate event.

```json
{ "action": "file-dropped", "data": { "path": "/home/user/photo.png" } }
```

| Field  | Description                     |
| ------ | ------------------------------- |
| `path` | Absolute path to the dropped file |
