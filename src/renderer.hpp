#pragma once

#include <string>

/**
 * @namespace Renderer
 * @brief Per-frame rendering logic and HTML content styling helpers.
 *
 * Call Renderer::MainLoopIteration() once per frame from the main loop.
 * It reads one pending action, updates audio, and draws the ImHTML canvas.
 */
namespace Renderer {

/**
 * @brief Builds the styled HTML string to feed into ImHTML::Canvas each frame.
 *
 * Prepends a <style> block that sets the base font size and correct intrinsic
 * heights for all ui-* custom elements, then appends the given content string.
 *
 * @param content The raw HTML content to wrap with element styles.
 * @return The complete HTML string ready for rendering.
 */
std::string BuildElementStyles(const std::string &content);

/**
 * @brief Executes one iteration of the main render loop.
 *
 * Drains all pending IO actions, processes file drops, ticks audio,
 * syncs the ImHTML base font size, then draws the full frame.
 */
void MainLoopIteration();

}  // namespace Renderer
