---
sidebar_position: 3
---

# `<ui-input>`

A single-line input field supporting text, number, integer, and password types.

```html
<ui-input id="username" type="text" placeholder="Enter name" value=""></ui-input>
<ui-input id="age" type="number" min="0" max="120" step="0.5" value="18"></ui-input>
<ui-input id="port" type="int" min="1" max="65535" step="1" value="8080"></ui-input>
<ui-input id="secret" type="password" placeholder="Enter password"></ui-input>
<ui-input id="search" type="text" live="true" placeholder="Search…"></ui-input>
```

## Attributes

| Attribute     | Required | Description                                                                                                               |
| ------------- | -------- | ------------------------------------------------------------------------------------------------------------------------- |
| `id`          | yes      | Element identifier                                                                                                        |
| `type`        | no       | `text` (default), `number`, `int`, or `password`                                                                          |
| `placeholder` | no       | Ghost text for `text` and `password` inputs                                                                               |
| `value`       | no       | Initial value                                                                                                             |
| `min` / `max` | no       | Clamp range for `number` and `int` inputs                                                                                 |
| `step`        | no       | Arrow-button increment for `number` and `int` inputs. Defaults to `1` for `int`; omit or `0` to hide arrows for `number` |
| `live`        | no       | `"true"` to emit `input-changed` on every keystroke instead of only on Enter or focus loss. Defaults to `"false"`         |

## Event

Emits [`input-changed`](../element-actions#input-changed) on Enter or focus loss by default, or on every keystroke when `live="true"`.
