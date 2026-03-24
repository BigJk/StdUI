---
sidebar_position: 7
---

# Pane Layout

## `set-pane-layout` — App → stdui

Replaces the current window layout with a new pane tree. The tree is a recursive structure of **pane leaves** and **split nodes**. Leaf panes are independently renderable areas; split nodes divide their allocated space among their children.

When `set-pane-layout` is not called, the layout defaults to a single pane with id `"main"`. The plain-string form of `update-content` always targets `"main"`, keeping it fully backwards-compatible with single-pane usage.

```json
{
  "action": "set-pane-layout",
  "data": {
    "type": "split",
    "direction": "horizontal",
    "children": [
      { "type": "pane", "id": "sidebar", "size": 1 },
      {
        "type": "split",
        "direction": "vertical",
        "size": 3,
        "children": [
          { "type": "pane", "id": "top" },
          { "type": "pane", "id": "bottom" }
        ]
      }
    ]
  }
}
```

## Node fields

| Field       | Required for | Description                                                                                                                     |
| ----------- | ------------ | ------------------------------------------------------------------------------------------------------------------------------- |
| `type`      | all nodes    | `"pane"` for a leaf or `"split"` for an interior divider                                                                        |
| `id`        | pane nodes   | Unique identifier used as the target for `update-content`                                                                       |
| `direction` | split nodes  | `"horizontal"` (left-to-right) or `"vertical"` (top-to-bottom)                                                                  |
| `children`  | split nodes  | Array of child nodes                                                                                                            |
| `size`      | all nodes    | Size hint for the parent split. **≤ 10**: relative weight. **> 10**: fixed pixel size. Defaults to `1`                          |

## Size semantics

- **`size <= 10`** — relative weight. After all fixed-pixel children are allocated, the remaining space is divided proportionally. A child with `size: 2` gets twice the flexible space of a child with `size: 1`.
- **`size > 10`** — fixed pixel size. The child is allocated exactly that many pixels along the split axis regardless of available space.

Mixed fixed and relative children are fully supported within the same split.
