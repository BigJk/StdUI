---
sidebar_position: 6
---

# Scroll Control

## `scroll-to` — App → stdui

Programmatically scrolls a pane to a given vertical position. The scroll is applied on the next rendered frame.

```json
{ "action": "scroll-to", "data": { "pane": "history", "position": "bottom" } }
```

**Fields:**

| Field      | Type              | Description                                                                                                                                                            |
| ---------- | ----------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `pane`     | string, required  | ID of the target pane                                                                                                                                                  |
| `position` | number or string  | Vertical scroll target. `"bottom"` scrolls to the end of content; a number is an absolute pixel offset from the top. Omitting `position` (or `0`) scrolls to the top  |

**Examples:**

```json
{ "action": "scroll-to", "data": { "pane": "chat",  "position": "bottom" } }
{ "action": "scroll-to", "data": { "pane": "log",   "position": 0 } }
{ "action": "scroll-to", "data": { "pane": "list",  "position": 320 } }
```
