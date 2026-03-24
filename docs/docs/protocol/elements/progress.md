---
sidebar_position: 8
---

# `<ui-progress>`

A read-only progress bar. The fill fraction is computed as `(value - min) / (max - min)`. Update the value at runtime via [`set-value`](/docs/protocol/element-actions#set-value).

```html
<ui-progress id="upload" value="0.4" min="0" max="1" overlay="40%"></ui-progress>
```

## Attributes

| Attribute | Required | Description                                                     |
| --------- | -------- | --------------------------------------------------------------- |
| `id`      | yes      | Element identifier; used with `set-value` to update the bar     |
| `value`   | no       | Current value (default `0`)                                     |
| `min`     | no       | Minimum value (default `0`)                                     |
| `max`     | no       | Maximum value (default `1`)                                     |
| `overlay` | no       | Text drawn on top of the bar (e.g. `"40%"`); omit for no label  |

## Event

Does not emit any events.
