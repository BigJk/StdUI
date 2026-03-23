#include "layout.hpp"

#include <memory>
#include <string>

namespace Layout {

/**
 * @brief Recursively parse a layout node from an Hjson value.
 *
 * @param value The Hjson value to parse.
 * @return The parsed node, or nullptr on failure.
 */
std::shared_ptr<Node> Parse(const Hjson::Value &value) {
  if (value.type() != Hjson::Type::Map) return nullptr;

  auto typeVal = value["type"];
  if (!typeVal.defined()) return nullptr;

  std::string typeStr = typeVal.to_string();
  auto node = std::make_shared<Node>();
  node->size = value["size"].defined() ? static_cast<float>(value["size"].to_double()) : 1.0f;

  if (typeStr == "pane") {
    node->type = Node::Type::Pane;
    node->id = value["id"].defined() ? value["id"].to_string() : "main";
    return node;
  }

  if (typeStr == "split") {
    node->type = Node::Type::Split;

    std::string dir = value["direction"].defined() ? value["direction"].to_string() : "horizontal";
    node->direction = (dir == "vertical") ? Direction::Vertical : Direction::Horizontal;

    auto children = value["children"];
    if (children.type() != Hjson::Type::Vector) return nullptr;

    for (int i = 0; i < static_cast<int>(children.size()); i++) {
      auto child = Parse(children[i]);
      if (child) {
        node->children.push_back(child);
      }
    }

    if (node->children.empty()) return nullptr;
    return node;
  }

  return nullptr;
}

/**
 * @brief Returns the default single-pane layout with id "main".
 */
std::shared_ptr<Node> Default() {
  auto node = std::make_shared<Node>();
  node->type = Node::Type::Pane;
  node->id = "main";
  node->size = 1.0f;
  return node;
}

}  // namespace Layout
