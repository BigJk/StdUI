#include "state.hpp"

std::unordered_map<std::string, std::string> ELEMENT_STATE;
std::unordered_map<std::string, std::string> PANE_CONTENT = {{"main", "Loading..."}};
std::shared_ptr<Layout::Node> LAYOUT = Layout::Default();
std::atomic<bool> CLOSE_REQUESTED{false};
std::string CURRENT_PANE = "main";
std::optional<ConfirmDialog> CONFIRM_PENDING;
std::unordered_map<std::string, ScrollRequest> SCROLL_REQUESTS;
std::vector<Toast> TOASTS;
std::unordered_map<std::string, Keybind> KEYBINDS;
