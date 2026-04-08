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

struct RenderedColumnRange
{
    int start = 0;
    int end   = 0;
};

// Approximate viewport size for the first render before FTXUI's reflect() has measured
// the actual box. Breakdown: window border (2) + header (1) + separators (2) + status lines (3).
// The value 7 slightly underestimates, which is acceptable as a fallback for a single frame.
constexpr int window_chrome_height = 7;

int estimate_visible_line_count(const ftxui::Box& viewport_box, int screen_height)
{
    if (viewport_box.y_max > viewport_box.y_min)
    {
        return viewport_box.y_max - viewport_box.y_min + 1;
    }

    return std::max(1, screen_height - window_chrome_height);
}

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

std::optional<RenderedColumnRange> preview_range_for_line(const std::string& rendered_line, int rendered_text_start_column,
                                                          const std::optional<HiddenColumnRange>& hide_columns_preview)
{
    if (!hide_columns_preview.has_value())
    {
        return std::nullopt;
    }

    const int highlight_start = rendered_text_start_column + hide_columns_preview->first_column - 1;
    const int highlight_end   = rendered_text_start_column + hide_columns_preview->last_column;
    const int clamped_start   = std::clamp(highlight_start, 0, static_cast<int>(rendered_line.size()));
    const int clamped_end     = std::clamp(highlight_end, clamped_start, static_cast<int>(rendered_line.size()));
    if (clamped_start >= clamped_end)
    {
        return std::nullopt;
    }

    return RenderedColumnRange {
        clamped_start,
        clamped_end,
    };
}

std::optional<RenderedColumnRange> selection_range_for_line(const std::string& rendered_line, int line_index,
                                                            const std::optional<std::pair<TextPosition, TextPosition>>& selected_range)
{
    if (!selected_range.has_value() || line_index < selected_range->first.line || line_index > selected_range->second.line)
    {
        return std::nullopt;
    }

    const int highlight_start = (line_index == selected_range->first.line) ? selected_range->first.column : 0;
    const int highlight_end =
        (line_index == selected_range->second.line) ? selected_range->second.column : static_cast<int>(rendered_line.size());
    const int clamped_start = std::clamp(highlight_start, 0, static_cast<int>(rendered_line.size()));
    const int clamped_end   = std::clamp(highlight_end, clamped_start, static_cast<int>(rendered_line.size()));
    if (clamped_start >= clamped_end)
    {
        return std::nullopt;
    }

    return RenderedColumnRange {
        clamped_start,
        clamped_end,
    };
}

ftxui::Element render_line_with_ranges(const std::string& rendered_line, const std::optional<RenderedColumnRange>& preview_range,
                                       const std::optional<RenderedColumnRange>& selection_range)
{
    if (!preview_range.has_value() && !selection_range.has_value())
    {
        return ftxui::text(rendered_line);
    }

    std::vector<int> boundaries;
    boundaries.reserve(6);
    boundaries.push_back(0);
    boundaries.push_back(static_cast<int>(rendered_line.size()));
    if (preview_range.has_value())
    {
        boundaries.push_back(preview_range->start);
        boundaries.push_back(preview_range->end);
    }
    if (selection_range.has_value())
    {
        boundaries.push_back(selection_range->start);
        boundaries.push_back(selection_range->end);
    }

    std::sort(boundaries.begin(), boundaries.end());
    boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());

    ftxui::Elements row;
    row.reserve(boundaries.size());

    for (std::size_t i = 0; i + 1 < boundaries.size(); ++i)
    {
        const int segment_start = boundaries[i];
        const int segment_end   = boundaries[i + 1];
        if (segment_start >= segment_end)
        {
            continue;
        }

        auto segment = ftxui::text(
            rendered_line.substr(static_cast<std::size_t>(segment_start), static_cast<std::size_t>(segment_end - segment_start)));

        const bool preview_active = preview_range.has_value() && segment_start < preview_range->end && segment_end > preview_range->start;
        const bool selection_active =
            selection_range.has_value() && segment_start < selection_range->end && segment_end > selection_range->start;

        if (preview_active)
        {
            segment |= ftxui::bgcolor(theme::hide_columns_preview_bg) | ftxui::color(theme::hide_columns_preview_fg);
        }

        if (selection_active)
        {
            segment |= ftxui::inverted;
        }

        row.push_back(std::move(segment));
    }

    return ftxui::hbox(std::move(row));
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

ftxui::Element LogView::render(const LogModel& model, const LogController& controller, const std::string& header_text, int screen_height,
                               std::optional<HiddenColumnRange> hide_columns_preview)
{
    const int visible_line_count = estimate_visible_line_count(_viewport_box, screen_height);
    const int first_visible_line = controller.first_visible_line_index(model, visible_line_count).value;

    ftxui::Elements content;
    content.reserve(static_cast<std::size_t>(std::max(1, visible_line_count)));

    if (model.line_count() == 0)
    {
        content.push_back(ftxui::hbox({
            ftxui::text("1 ") | ftxui::color(theme::muted),
            ftxui::text(model.total_line_count() == 0 ? "<empty file>" : "<no matching lines>") | ftxui::color(theme::muted),
        }));
    }
    else
    {
        const auto selected_range    = controller.selection_bounds(model);
        const auto effective_preview = selected_range.has_value() ? std::optional<HiddenColumnRange>() : hide_columns_preview;
        const auto active_find_index = controller.active_find_visible_index(model);
        const auto rendered_lines    = model.rendered_lines(first_visible_line, visible_line_count);

        for (std::size_t offset = 0; offset < rendered_lines.size(); ++offset)
        {
            const int index            = first_visible_line + static_cast<int>(offset);
            const bool is_find_match   = model.find_active() && model.visible_line_matches_find(index);
            const bool is_active_match = active_find_index.has_value() && active_find_index->value == index;
            const auto& rendered_line  = rendered_lines[offset];

            const auto preview_range   = preview_range_for_line(rendered_line, model.rendered_text_start_column(index), effective_preview);
            const auto selection_range = selection_range_for_line(rendered_line, index, selected_range);
            auto styled_row            = render_line_with_ranges(rendered_line, preview_range, selection_range);
            if (is_find_match)
            {
                styled_row = theme::apply_find_highlight(std::move(styled_row), is_active_match);
            }

            content.push_back(std::move(styled_row));
        }
    }

    // Scrollbar with visible track
    ftxui::Element scrollbar = ftxui::text("");
    if (model.line_count() > visible_line_count)
    {
        const int total_lines = model.line_count();
        const int thumb_size  = std::max(1, (visible_line_count * visible_line_count) / std::max(total_lines, visible_line_count));
        const int track_size  = std::max(1, visible_line_count - thumb_size);
        const int max_offset  = std::max(1, total_lines - visible_line_count);
        const int thumb_top   = (first_visible_line * track_size) / max_offset;

        ftxui::Elements track;
        track.reserve(static_cast<std::size_t>(visible_line_count));
        for (int row = 0; row < visible_line_count; ++row)
        {
            const bool is_thumb = row >= thumb_top && row < (thumb_top + thumb_size);
            track.push_back(ftxui::text(is_thumb ? "\xe2\x94\x83" : "\xe2\x94\x82") |
                            ftxui::color(is_thumb ? theme::scrollbar_thumb_fg : theme::scrollbar_track_fg));
        }

        scrollbar = ftxui::vbox(std::move(track));
    }

    auto log_view = ftxui::hbox({
                        ftxui::vbox(std::move(content)) | ftxui::reflect(_viewport_box) | ftxui::flex,
                        scrollbar,
                    }) |
                    ftxui::flex;

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
    return estimate_visible_line_count(_viewport_box, screen_height);
}

std::optional<TextPosition> LogView::mouse_to_text_position(const LogModel& model, const LogController& controller,
                                                            const ftxui::Mouse& mouse) const
{
    if (model.line_count() == 0)
    {
        return std::nullopt;
    }

    if (mouse.x < _viewport_box.x_min || mouse.x > _viewport_box.x_max || mouse.y < _viewport_box.y_min || mouse.y > _viewport_box.y_max)
    {
        return std::nullopt;
    }

    const int viewport_line_count = std::max(1, _viewport_box.y_max - _viewport_box.y_min + 1);
    const int line_index          = controller.first_visible_line_index(model, viewport_line_count).value + (mouse.y - _viewport_box.y_min);
    if (line_index < 0 || line_index >= model.line_count())
    {
        return std::nullopt;
    }

    const auto line        = model.rendered_line(line_index);
    const int mouse_column = mouse.x - _viewport_box.x_min;
    return TextPosition {
        line_index,
        byte_index_for_cell_column(line, mouse_column),
    };
}

} // namespace slayerlog
