---
sidebar_position: 1
sidebar_label: Overview
---

# Interactive Elements

Interactive elements are custom HTML tags rendered by ImGui. They use the `ui-` prefix to avoid conflicts with litehtml's built-in CSS rules for standard HTML tags (e.g. `<ui-button>`, `<ui-input>`).

## State

Each element requires an `id` attribute used to track state and identify events. State is seeded from the `value` / `checked` attribute on first render and persisted internally until changed by the user or overridden via a [`set-value`](/docs/protocol/element-actions#set-value) action.

## Events

Most elements emit [`input-changed`](/docs/protocol/element-actions#input-changed) when their value changes. Buttons emit [`button-clicked`](/docs/protocol/element-actions#button-clicked).

## Elements

| Element | Description |
| ------- | ----------- |
| [`<ui-button>`](/docs/protocol/elements/button) | Clickable button |
| [`<ui-input>`](/docs/protocol/elements/input) | Text, number, int, or password input field |
| [`<ui-select>`](/docs/protocol/elements/select) | Dropdown selector |
| [`<ui-slider>`](/docs/protocol/elements/slider) | Float or integer slider |
| [`<ui-checkbox>`](/docs/protocol/elements/checkbox) | Toggle checkbox |
| [`<ui-textarea>`](/docs/protocol/elements/textarea) | Multi-line text editor |
| [`<ui-progress>`](/docs/protocol/elements/progress) | Read-only progress bar |
| [`<ui-color>`](/docs/protocol/elements/color) | Inline color picker |
| [`<ui-file-select>`](/docs/protocol/elements/file-select) | Native file-open dialog |
| [`<ui-folder-select>`](/docs/protocol/elements/folder-select) | Native folder-picker dialog |
| [`<ui-file-save>`](/docs/protocol/elements/file-save) | Native file-save dialog |
