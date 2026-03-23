#pragma once

/**
 * @namespace Elements
 * @brief Registration of all custom ImHTML elements.
 */
namespace Elements {

/**
 * @brief Registers all ui-* custom elements with ImHTML.
 * Must be called once after ImHTML is initialised, before the main loop.
 */
void RegisterAll();

}  // namespace Elements
