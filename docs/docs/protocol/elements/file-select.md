---
sidebar_position: 10
---

# `<ui-file-select>`

Renders a read-only path display next to a button. Clicking the button opens a native OS file-open dialog synchronously. If the user cancels, no event is emitted.

```html
<ui-file-select id="input-file" label="Browse..." filter="png,jpg" value=""></ui-file-select>
```

## Attributes

| Attribute | Required | Description                                                                       |
| --------- | -------- | --------------------------------------------------------------------------------- |
| `id`      | yes      | Element identifier                                                                |
| `label`   | no       | Button text (default `"Browse..."`)                                               |
| `filter`  | no       | Comma-separated file extensions to filter, e.g. `"png,jpg"` (default: all files)  |
| `value`   | no       | Initial path shown in the display                                                 |

## Event

Emits [`input-changed`](../element-actions#input-changed) with the selected absolute file path as the value.
