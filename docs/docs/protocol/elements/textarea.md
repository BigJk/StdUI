---
sidebar_position: 7
---

# `<ui-textarea>`

A multi-line text editor. Supports **Tab** for indentation; **Enter** inserts a newline.

```html
<ui-textarea id="notes" placeholder="Enter notes..." rows="4" value=""></ui-textarea>
<ui-textarea id="live-notes" live="true" placeholder="Live notes..."></ui-textarea>
```

## Attributes

| Attribute     | Required | Description                                                                                               |
| ------------- | -------- | --------------------------------------------------------------------------------------------------------- |
| `id`          | yes      | Element identifier                                                                                        |
| `placeholder` | no       | Ghost text shown when empty                                                                               |
| `value`       | no       | Initial content                                                                                           |
| `rows`        | no       | Visual row hint (default `4`); override actual height via CSS `height` in your content's `<style>` block  |
| `live`        | no       | `"true"` to emit `input-changed` on every keystroke instead of only on focus loss. Defaults to `"false"`  |

## Event

Emits [`input-changed`](../element-actions#input-changed) on focus loss (default), or on every keystroke when `live="true"`.
