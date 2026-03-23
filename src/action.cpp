#include "action.hpp"

namespace Action {

std::optional<std::string> Type(const Hjson::Value &value) {
  auto field = value["action"];
  if (!field.defined() || field.type() == Hjson::Type::Undefined || field.type() == Hjson::Type::Null) {
    return std::nullopt;
  }
  return field.to_string();
}

std::optional<Hjson::Value> Data(const Hjson::Value &value) {
  auto field = value["data"];
  if (!field.defined() || field.type() == Hjson::Type::Undefined || field.type() == Hjson::Type::Null) {
    return std::nullopt;
  }
  return field;
}

std::optional<Hjson::Value> ExpectData(const Hjson::Value &value, const std::string &type) {
  auto t = Type(value);
  if (!t || *t != type) {
    return std::nullopt;
  }
  return Data(value);
}

}  // namespace Action
