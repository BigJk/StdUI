---
sidebar_position: 11
---

# `<ui-folder-select>`

Same as [`<ui-file-select>`](./file-select) but opens a native folder-picker dialog instead of a file dialog.

```html
<ui-folder-select id="output-dir" label="Browse..." value=""></ui-folder-select>
```

## Attributes

| Attribute | Required | Description                          |
| --------- | -------- | ------------------------------------ |
| `id`      | yes      | Element identifier                   |
| `label`   | no       | Button text (default `"Browse..."`)  |
| `value`   | no       | Initial path shown in the display    |

## Event

Emits [`input-changed`](../element-actions#input-changed) with the selected absolute folder path as the value.
