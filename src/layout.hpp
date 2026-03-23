#pragma once

#include <hjson.h>

#include <memory>
#include <string>
#include <vector>

namespace Layout {

/**
 * @brief The split direction for a split node.
 */
enum class Direction {
  Horizontal,  ///< Children are arranged left-to-right.
  Vertical,    ///< Children are arranged top-to-bottom.
};

/**
 * @brief A single node in the pane layout tree.
 *
 * A node is either a leaf pane (type == Pane) or an interior split
 * (type == Split) that subdivides its allocated rect among its children.
 */
struct Node {
  enum class Type { Pane, Split };

  Type type;
  /**
   * Size hint interpreted by the parent split node.
   *
   * - **size <= 10** — treated as a relative weight (same as before).
   *   The available space after all fixed children are subtracted is divided
   *   proportionally among relative-weight children.
   * - **size > 10** — treated as a fixed pixel size.  The child is allocated
   *   exactly that many pixels regardless of the available space.
   *
   * Defaults to 1.0 (relative weight of 1).
   */
  float size;

  // Pane fields (type == Pane)
  std::string id;  ///< Unique pane identifier used to target update-content.

  // Split fields (type == Split)
  Direction direction;
  std::vector<std::shared_ptr<Node>> children;
};

/**
 * @brief Parse a layout tree from an Hjson value.
 *
 * The expected format is a recursive object:
 * @code
 * { "type": "pane", "id": "main" }
 * { "type": "split", "direction": "horizontal", "children": [...] }
 * @endcode
 *
 * @param value The Hjson value to parse.
 * @return The root node, or nullptr if parsing fails.
 */
std::shared_ptr<Node> Parse(const Hjson::Value &value);

/**
 * @brief Returns the default single-pane layout with id "main".
 */
std::shared_ptr<Node> Default();

}  // namespace Layout
