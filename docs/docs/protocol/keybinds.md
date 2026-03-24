---
sidebar_position: 5
---

# Keybinds

## `set-keybinds` — App → stdui

Registers a set of keyboard shortcuts. Calling this action **replaces** any previously registered shortcuts entirely. When a registered shortcut is triggered, stdui emits a `key-pressed` event.

To clear all keybinds, send an empty array.

```json
{
  "action": "set-keybinds",
  "data": [
    { "id": "save",     "key": "s",   "ctrl": true },
    { "id": "find",     "key": "f",   "ctrl": true },
    { "id": "quit",     "key": "q",   "ctrl": true },
    { "id": "help",     "key": "f1" },
    { "id": "new-item", "key": "n",   "ctrl": true, "shift": true }
  ]
}
```

**Fields per entry:**

| Field   | Required | Description                                                              |
| ------- | -------- | ------------------------------------------------------------------------ |
| `id`    | yes      | Application-defined identifier echoed back in `key-pressed`              |
| `key`   | yes      | Primary key name (case-insensitive). See [Key Names](#key-names) below   |
| `ctrl`  | no       | Require Ctrl (left or right). Defaults to `false`                        |
| `shift` | no       | Require Shift (left or right). Defaults to `false`                       |
| `alt`   | no       | Require Alt (left or right). Defaults to `false`                         |
| `meta`  | no       | Require Super / Cmd (left or right). Defaults to `false`                 |

### Key Names

Single characters are accepted as-is: `"a"`–`"z"`, `"0"`–`"9"`, `"/"`, `","`, `"."`, `"-"`, `"="`, `";"`, `"'"`, `"["`, `"]"`, `"\\"`, `` "`" ``.

Named keys:

| Name                 | Key           |
| -------------------- | ------------- |
| `space`              | Space bar     |
| `enter` / `return`   | Enter         |
| `tab`                | Tab           |
| `backspace`          | Backspace     |
| `delete` / `del`     | Delete        |
| `insert`             | Insert        |
| `escape` / `esc`     | Escape        |
| `left`               | Arrow left    |
| `right`              | Arrow right   |
| `up`                 | Arrow up      |
| `down`               | Arrow down    |
| `home`               | Home          |
| `end`                | End           |
| `pageup`             | Page Up       |
| `pagedown`           | Page Down     |
| `capslock`           | Caps Lock     |
| `scrolllock`         | Scroll Lock   |
| `numlock`            | Num Lock      |
| `printscreen`        | Print Screen  |
| `pause`              | Pause         |
| `f1`–`f12`           | Function keys |

---

## `key-pressed` — stdui → App

Emitted when a registered keyboard shortcut is activated.

```json
{ "action": "key-pressed", "data": { "id": "save" } }
```

| Field | Description                                                         |
| ----- | ------------------------------------------------------------------- |
| `id`  | The application-defined identifier from the matching `set-keybinds` entry |
