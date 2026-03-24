---
sidebar_position: 4
---

# `<ui-select>`

A dropdown selector rendered by ImGui.

```html
<ui-select id="theme" options="Light|Dark|System" value="Light"></ui-select>
```

## Attributes

| Attribute | Required | Description                                                                                |
| --------- | -------- | ------------------------------------------------------------------------------------------ |
| `id`      | yes      | Element identifier                                                                         |
| `options` | yes      | Pipe-separated (`\|`) list of options. Use `\\\|` to include a literal `\|` in an option.  |
| `value`   | no       | Initially selected option (defaults to first)                                              |

## Event

Emits [`input-changed`](../element-actions#input-changed) immediately on selection change.
