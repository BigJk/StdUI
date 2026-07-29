---
sidebar_position: 2
sidebar_label: Protocol
---

# Protocol

StdUI communicates with the controlling application using line-delimited JSON. Every message is a single line of JSON terminated by a newline character.

By default the channel is **stdin/stdout**, but Unix domain sockets and named pipes are also available — see [IPC Transports](/docs/ipc-transports). The message format is identical regardless of the transport.

## Message Format

```json
{
  "action": "action_type",
  "data": {
    /* optional data payload */
  }
}
```

| Field    | Type             | Description                              |
| -------- | ---------------- | ---------------------------------------- |
| `action` | string, required | The type of action being sent            |
| `data`   | any, optional    | Action-specific payload (object, string, or array) |

Action names use kebab-case (e.g. `update-content`, `window-closed`).

## Minimal round-trip

```
App → stdui   {"action":"settings","data":{"title":"Hello","windowWidth":400,"windowHeight":300}}
stdui → App   {"action":"ready"}
App → stdui   {"action":"update-content","data":"<h1>Hi</h1><ui-button text=\"Click me\" action=\"hi\"></ui-button>"}
  ... user clicks the button ...
stdui → App   {"action":"button-clicked","data":{"action":"hi","text":"Click me","pane":"main"}}
App → stdui   {"action":"update-content","data":"<h1>You clicked it!</h1>"}
  ... user closes the window ...
stdui → App   {"action":"window-closed"}
```

## Sections

- [**Core Actions**](/docs/protocol/actions) — `settings`, `update-content`, lifecycle events
- [**Interactive Elements**](/docs/protocol/elements/) — all `<ui-*>` tags
- [**Element Actions**](/docs/protocol/element-actions) — `set-value`, `get-value`, `input-changed`, `button-clicked`
- [**Window Control**](/docs/protocol/window) — title, icon, position, minimize/maximize, toast, clipboard
- [**Keybinds**](/docs/protocol/keybinds) — `set-keybinds`, `key-pressed`
- [**Scroll Control**](/docs/protocol/scroll) — `scroll-to`
- [**Pane Layout**](/docs/protocol/pane-layout) — `set-pane-layout`
