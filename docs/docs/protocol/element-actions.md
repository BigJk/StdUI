---
sidebar_position: 3
---

# Element Actions

Actions for reading and writing interactive element state, and the events they emit.

## `input-changed` — stdui → App

Emitted when the user commits a change to any interactive element.

```json
{ "action": "input-changed", "data": { "id": "volume", "value": "0.75", "pane": "main" } }
```

| Field   | Description                                               |
| ------- | --------------------------------------------------------- |
| `id`    | The element's `id` attribute                              |
| `value` | The new value as a string. For checkboxes: `"true"` or `"false"` |
| `pane`  | The id of the pane the element belongs to                 |

---

## `button-clicked` — stdui → App

Emitted when a [`<ui-button>`](./elements/button) is clicked. `data` contains all HTML attributes of the button plus a `pane` field.

```json
{ "action": "button-clicked", "data": { "action": "save", "text": "Save", "pane": "main" } }
```

| Field  | Description                                           |
| ------ | ----------------------------------------------------- |
| `pane` | The id of the pane the button belongs to              |
| …      | All other HTML attributes of the `<ui-button>` element |

---

## `set-value` — App → stdui

Programmatically set the value of an element by id.

```json
{ "action": "set-value", "data": { "id": "username", "value": "Alice" } }
```

---

## `get-value` — App → stdui

Request the current value of an element by id. stdui responds with a `value-result` event.

The `data` field may be a plain string (the id) or an object with an `id` field:

```json
{ "action": "get-value", "data": "username" }
{ "action": "get-value", "data": { "id": "username" } }
```

---

## `value-result` — stdui → App

Emitted in response to a `get-value` request.

```json
{ "action": "value-result", "data": { "id": "username", "value": "Alice" } }
```

| Field   | Description                                                              |
| ------- | ------------------------------------------------------------------------ |
| `id`    | The element's `id` attribute                                             |
| `value` | Current value as a string, or `""` if the id has never been rendered or set |
