---
sidebar_position: 2
---

# `<ui-button>`

A clickable button rendered by ImGui.

```html
<ui-button text="Click Me" action="my-action" tooltip="Optional"></ui-button>
```

## Attributes

| Attribute | Required | Description                                    |
| --------- | -------- | ---------------------------------------------- |
| `text`    | yes      | Button label                                   |
| `action`  | yes      | Identifier sent back in `button-clicked` data  |
| `tooltip` | no       | Hover tooltip text                             |

## Event

Emits [`button-clicked`](/docs/protocol/element-actions#button-clicked) when clicked. The event `data` contains all HTML attributes of the button plus a `pane` field.

```json
{ "action": "button-clicked", "data": { "action": "my-action", "text": "Click Me", "pane": "main" } }
```
