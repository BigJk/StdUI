---
sidebar_position: 6
---

# `<ui-checkbox>`

A toggle checkbox with an optional label.

```html
<ui-checkbox id="notifications" label="Enable notifications" checked="true"></ui-checkbox>
```

## Attributes

| Attribute | Required | Description                                               |
| --------- | -------- | --------------------------------------------------------- |
| `id`      | yes      | Element identifier                                        |
| `label`   | no       | Label shown next to the checkbox                          |
| `checked` | no       | Initial state: `"true"` or `"false"` (default `"false"`)  |

## Event

Emits [`input-changed`](/docs/protocol/element-actions#input-changed) immediately on toggle. Value is `"true"` or `"false"`.
