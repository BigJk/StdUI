---
sidebar_position: 12
---

# `<ui-file-save>`

Renders a read-only path display next to a button. Clicking the button opens a native OS save-file dialog synchronously. If the user cancels, no event is emitted.

```html
<ui-file-save id="output-file" label="Save..." filter="png,jpg" value=""></ui-file-save>
```

## Attributes

| Attribute | Required | Description                                                                       |
| --------- | -------- | --------------------------------------------------------------------------------- |
| `id`      | yes      | Element identifier                                                                |
| `label`   | no       | Button text (default `"Save..."`)                                                 |
| `filter`  | no       | Comma-separated file extensions to filter, e.g. `"png,jpg"` (default: all files)  |
| `value`   | no       | Initial path shown in the display                                                 |

## Event

Emits [`input-changed`](../element-actions#input-changed) with the chosen absolute file path as the value.
