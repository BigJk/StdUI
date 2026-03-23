#pragma once

#include <hjson.h>

#include <optional>
#include <string>

namespace Action {

/**
 * @brief Extract the action type from an Hjson value.
 *
 * Checks for the presence of the "action" field and returns it as a string.
 *
 * @param value The Hjson value to inspect.
 * @return The action type string, or std::nullopt if the field is absent.
 */
std::optional<std::string> Type(const Hjson::Value &value);

/**
 * @brief Extract the data payload from an Hjson value.
 *
 * Checks for the presence of the "data" field and returns it as an Hjson::Value.
 *
 * @param value The Hjson value to inspect.
 * @return The data payload, or std::nullopt if the field is absent.
 */
std::optional<Hjson::Value> Data(const Hjson::Value &value);

/**
 * @brief Extract the data payload if the action type matches the expected type.
 *
 * Returns the "data" field only if the "action" field equals the given type.
 *
 * @param value The Hjson value to inspect.
 * @param type  The expected action type string.
 * @return The data payload, or std::nullopt if the type does not match or fields are absent.
 */
std::optional<Hjson::Value> ExpectData(const Hjson::Value &value, const std::string &type);

}  // namespace Action
