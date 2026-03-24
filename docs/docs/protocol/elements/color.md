---
sidebar_position: 9
---

# `<ui-color>`

An inline color swatch with a hex input field (ImGui `ColorEdit3` / `ColorEdit4`). Clicking the swatch opens an expanded color picker popup.

```html
<ui-color id="accent" value="#0969da"></ui-color>
<ui-color id="overlay" value="#0969da80" alpha="true"></ui-color>
```

## Attributes

| Attribute | Required | Description                                                                                               |
| --------- | -------- | --------------------------------------------------------------------------------------------------------- |
| `id`      | yes      | Element identifier                                                                                        |
| `value`   | no       | Initial color as a hex string. `#RRGGBB` when `alpha="false"` (default), `#RRGGBBAA` when `alpha="true"` |
| `alpha`   | no       | `"true"` to enable the alpha channel. Defaults to `"false"`                                               |

## Event

Emits [`input-changed`](../element-actions#input-changed) immediately on every color change. The value is always a lowercase hex string: `#rrggbb` or `#rrggbbaa`.
