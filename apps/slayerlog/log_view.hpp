#pragma once

#include <optional>
#include <cstddef>
#include <string>
#include <vector>

#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/elements.hpp>

#include "log_controller.hpp"
#include "log_model.hpp"
#include "log_text_view.hpp"

namespace slayerlog
{

class LogView
{
public:
    int visible_line_count(int screen_height) const;
    ftxui::Element render(const LogModel& model, LogController& controller, const std::string& header_text, int screen_height,
                          std::optional<HiddenColumnRange> hide_columns_preview = std::nullopt);
    std::optional<TextPosition> mouse_to_text_position(const LogModel& model, const LogController& controller,
                                                       const ftxui::Mouse& mouse) const;

private:
    LogTextView _text_view;
};

} // namespace slayerlog
