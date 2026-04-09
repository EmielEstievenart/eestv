#include "log_view.hpp"
#include "view_theme.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include <ftxui/screen/string.hpp>

namespace slayerlog
{

namespace
{

// Approximate viewport size for the first render before FTXUI's reflect() has measured
// the actual box. Breakdown: window border (2) + header (1) + separators (2) + status lines (3)
// + horizontal scrollbar (1).
constexpr int window_chrome_height = 8;

std::string join(const std::vector<std::string>& items, const std::string& separator = ", ")
{
    std::string result;
    for (std::size_t i = 0; i < items.size(); ++i)
    {
        if (i > 0)
        {
            result += separator;
        }
        result += items[i];
    }
    return result;
}

ftxui::Element build_filter_status(const LogModel& model)
{
    ftxui::Elements parts;
    parts.push_back(theme::badge("FILTER", theme::label_filter_fg));

    const auto hidden_before  = model.hidden_before_line_number();
    const auto hidden_columns = model.hidden_columns();

    if (model.include_filters().empty() && model.exclude_filters().empty())
    {
        parts.push_back(ftxui::text(" none") | ftxui::color(theme::muted));
        if (hidden_before.has_value())
        {
            parts.push_back(ftxui::text(" | before line " + std::to_string(*hidden_before)) | ftxui::color(theme::muted));
        }

        if (hidden_columns.has_value())
        {
            parts.push_back(ftxui::text(" | columns " + std::to_string(hidden_columns->first_column) + "-" +
                                        std::to_string(hidden_columns->last_column)) |
                            ftxui::color(theme::muted));
        }

        return ftxui::hbox(std::move(parts));
    }

    if (!model.include_filters().empty())
    {
        parts.push_back(ftxui::text(" in(" + join(model.include_filters()) + ")"));
    }

    if (!model.exclude_filters().empty())
    {
        parts.push_back(ftxui::text(" out(" + join(model.exclude_filters()) + ")"));
    }

    if (hidden_before.has_value())
    {
        parts.push_back(ftxui::text(" | before line " + std::to_string(*hidden_before)) | ftxui::color(theme::muted));
    }

    if (hidden_columns.has_value())
    {
        parts.push_back(
            ftxui::text(" | columns " + std::to_string(hidden_columns->first_column) + "-" + std::to_string(hidden_columns->last_column)) |
            ftxui::color(theme::muted));
    }

    return ftxui::hbox(std::move(parts));
}

int byte_index_for_cell_column(const std::string& text, int cell_column)
{
    const int max_cells        = ftxui::string_width(text);
    const int clamped_cell_col = std::clamp(cell_column, 0, max_cells);
    if (clamped_cell_col == 0)
    {
        return 0;
    }

    auto next_codepoint_end = [&](std::size_t start)
    {
        std::size_t end = std::min(start + 1, text.size());
        while (end < text.size() && (static_cast<unsigned char>(text[end]) & 0xC0U) == 0x80U)
        {
            ++end;
        }

        return end;
    };

    std::size_t best_byte_index = 0;
    std::size_t index           = 0;
    while (index < text.size())
    {
        const std::size_t next_index = next_codepoint_end(index);
        const int prefix_width       = ftxui::string_width(std::string_view(text.data(), next_index));
        if (prefix_width > clamped_cell_col)
        {
            break;
        }

        best_byte_index = next_index;
        index           = next_index;
    }

    return static_cast<int>(best_byte_index);
}

ftxui::Element build_find_status(const LogModel& model, const LogController& controller)
{
    ftxui::Elements parts;
    parts.push_back(theme::badge("FIND", theme::label_find_fg));

    if (!model.find_active())
    {
        parts.push_back(ftxui::text(" off") | ftxui::color(theme::muted));
        return ftxui::hbox(std::move(parts));
    }

    parts.push_back(ftxui::text(" \"" + model.find_query() + "\""));
    parts.push_back(ftxui::text(" " + std::to_string(model.visible_find_match_count()) + "/" +
                                std::to_string(model.total_find_match_count()) + " matches") |
                    ftxui::color(theme::muted));

    const auto active_visible_index = controller.active_find_visible_index(model);
    if (active_visible_index.has_value())
    {
        parts.push_back(ftxui::text(" | line " + std::to_string(active_visible_index->value + 1)) | ftxui::color(theme::muted));
    }

    return ftxui::hbox(std::move(parts));
}

ftxui::Element build_key_hints()
{
    auto sep = []() { return ftxui::text("  "); };
    return ftxui::hbox({
        theme::key_hint("Ctrl+P", "commands"),
        sep(),
        theme::key_hint("Ctrl+R", "history"),
        sep(),
        theme::key_hint("\xe2\x86\x92", "next"),
        sep(),
        theme::key_hint("\xe2\x86\x90", "prev"),
        sep(),
        theme::key_hint("Esc", "close find"),
        sep(),
        theme::key_hint("q", "quit"),
    });
}

} // namespace

ftxui::Element LogView::render(const LogModel& model, LogController& controller, const std::string& header_text, int screen_height,
                               std::optional<HiddenColumnRange> hide_columns_preview)
{
    const int fallback_line_count = std::max(1, screen_height - window_chrome_height);
    const int visible_line_count  = _text_view.visible_line_count(fallback_line_count);

    controller.update_viewport_col_count(_text_view.visible_col_count());
    const auto text_view_data = controller.render_data(model, visible_line_count, hide_columns_preview);
    auto log_view             = _text_view.render(text_view_data) | ftxui::flex;

    // Header with optional paused indicator
    ftxui::Element header;
    if (model.updates_paused())
    {
        header = ftxui::hbox({
            ftxui::text(header_text) | ftxui::bold,
            ftxui::text(" "),
            theme::badge("PAUSED", theme::paused_fg),
        });
    }
    else
    {
        header = ftxui::text(header_text) | ftxui::bold;
    }

    return ftxui::window(ftxui::text("Slayerlog"), ftxui::vbox({
                                                       header,
                                                       ftxui::separator(),
                                                       log_view,
                                                       ftxui::separator(),
                                                       build_filter_status(model),
                                                       build_find_status(model, controller),
                                                       build_key_hints(),
                                                   }));
}

int LogView::visible_line_count(int screen_height) const
{
    return _text_view.visible_line_count(std::max(1, screen_height - window_chrome_height));
}

std::optional<TextPosition> LogView::mouse_to_text_position(const LogModel& model, const LogController& controller,
                                                            const ftxui::Mouse& mouse) const
{
    if (model.line_count() == 0)
    {
        return std::nullopt;
    }

    const auto& viewport_box = _text_view.content_box();
    if (mouse.x < viewport_box.x_min || mouse.x > viewport_box.x_max || mouse.y < viewport_box.y_min || mouse.y > viewport_box.y_max)
    {
        return std::nullopt;
    }

    const int viewport_line_count = std::max(1, viewport_box.y_max - viewport_box.y_min + 1);
    const int line_index          = controller.first_visible_line_index(model, viewport_line_count).value + (mouse.y - viewport_box.y_min);
    if (line_index < 0 || line_index >= model.line_count())
    {
        return std::nullopt;
    }

    const auto line        = model.rendered_line(line_index);
    const int first_visible_col = controller.first_visible_col(model);
    const int mouse_column = mouse.x - viewport_box.x_min;
    const std::string view_slice = first_visible_col >= static_cast<int>(line.size())
                                       ? std::string()
                                       : line.substr(static_cast<std::size_t>(first_visible_col));
    return TextPosition {
        line_index,
        first_visible_col + byte_index_for_cell_column(view_slice, mouse_column),
    };
}

} // namespace slayerlog
