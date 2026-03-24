---
sidebar_position: 5
---

# `<ui-slider>`

A horizontal slider for float or integer values.

```html
<ui-slider id="volume" min="0" max="1" value="0.5"></ui-slider>
<ui-slider id="speed" min="0" max="200" value="100" type="int" format="%d km/h"></ui-slider>
```

## Attributes

| Attribute | Required | Description                                                                                              |
| --------- | -------- | -------------------------------------------------------------------------------------------------------- |
| `id`      | yes      | Element identifier                                                                                       |
| `min`     | no       | Minimum value (default `0`)                                                                              |
| `max`     | no       | Maximum value (default `1`)                                                                              |
| `value`   | no       | Initial value                                                                                            |
| `type`    | no       | `float` (default) or `int`                                                                               |
| `format`  | no       | `printf`-style format string for the label. Defaults to `"%.3f"` for float and `"%d"` for int            |

## Event

Emits [`input-changed`](/docs/protocol/element-actions#input-changed) on mouse release. The emitted value is an integer string when `type="int"`, a float string otherwise.
