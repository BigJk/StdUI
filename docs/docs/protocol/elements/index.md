---
sidebar_position: 1
sidebar_label: Overview
---

# Interactive Elements

Interactive elements are custom HTML tags rendered by ImGui. They use the `ui-` prefix to avoid conflicts with litehtml's built-in CSS rules for standard HTML tags (e.g. `<ui-button>`, `<ui-input>`).

## State

Each element requires an `id` attribute used to track state and identify events. State is seeded from the `value` / `checked` attribute on first render and persisted internally until changed by the user or overridden via a [`set-value`](../element-actions#set-value) action.

## Events

Most elements emit [`input-changed`](../element-actions#input-changed) when their value changes. Buttons emit [`button-clicked`](../element-actions#button-clicked).

## Elements

| Element | Description |
| ------- | ----------- |
| [`<ui-button>`](./button) | Clickable button |
| [`<ui-input>`](./input) | Text, number, int, or password input field |
| [`<ui-select>`](./select) | Dropdown selector |
| [`<ui-slider>`](./slider) | Float or integer slider |
| [`<ui-checkbox>`](./checkbox) | Toggle checkbox |
| [`<ui-textarea>`](./textarea) | Multi-line text editor |
| [`<ui-progress>`](./progress) | Read-only progress bar |
| [`<ui-color>`](./color) | Inline color picker |
| [`<ui-file-select>`](./file-select) | Native file-open dialog |
| [`<ui-folder-select>`](./folder-select) | Native folder-picker dialog |
| [`<ui-file-save>`](./file-save) | Native file-save dialog |
