#include "log_text_view.hpp"

#include <algorithm>
#include <utility>

#include "view_theme.hpp"

namespace slayerlog
{

namespace
{

int measured_line_count(const ftxui::Box& box, int fallback_line_count)
{
    if (box.y_max > box.y_min)
    {
        return box.y_max - box.y_min + 1;
    }

    return std::max(1, fallback_line_count);
}

int measured_col_count(const ftxui::Box& box)
{
    if (box.x_max > box.x_min)
    {
        return box.x_max - box.x_min + 1;
    }

    return 1;
}

int effective_viewport_line_count(const LogTextViewRenderData& data)
{
    return std::max(1, data.viewport_line_count);
}

bool range_overlaps_segment(const LogTextViewStyledRange& range, int segment_start, int segment_end, LogTextViewRangeStyle style)
{
    return range.style == style && segment_start < range.col_end && segment_end > range.col_start;
}

} // namespace

ftxui::Element LogTextView::render_scrollbar(const LogTextViewRenderData& data)
{
    const int viewport_lines = effective_viewport_line_count(data);
    if (data.total_lines <= viewport_lines)
    {
        return ftxui::text("");
    }

    const int track_height = viewport_lines;
    const int thumb_height = std::max(1, (viewport_lines * viewport_lines) / std::max(data.total_lines, viewport_lines));
    const int max_offset   = std::max(1, data.total_lines - viewport_lines);
    const int track_range  = std::max(0, track_height - thumb_height);
    const int thumb_top    = (data.first_visible_line * track_range) / max_offset;

    ftxui::Elements track;
    track.reserve(static_cast<std::size_t>(track_height));
    for (int row = 0; row < track_height; ++row)
    {
        const bool is_thumb = row >= thumb_top && row < (thumb_top + thumb_height);
        track.push_back(ftxui::text(is_thumb ? "|" : ".") |
                        ftxui::color(is_thumb ? theme::scrollbar_thumb_fg : theme::scrollbar_track_fg));
    }

    return ftxui::vbox(std::move(track));
}

ftxui::Element LogTextView::render_hscrollbar(const LogTextViewRenderData& data)
{
    const int viewport_cols = std::max(1, data.viewport_col_count);
    if (data.max_line_width <= viewport_cols)
    {
        return ftxui::text("") | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 0);
    }

    const int track_width = viewport_cols;
    const int thumb_width = std::max(1, (viewport_cols * viewport_cols) / std::max(data.max_line_width, viewport_cols));
    const int max_offset  = std::max(1, data.max_line_width - viewport_cols);
    const int track_range = std::max(0, track_width - thumb_width);
    const int thumb_left  = (data.first_visible_col * track_range) / max_offset;

    ftxui::Elements track;
    track.reserve(static_cast<std::size_t>(track_width));
    for (int col = 0; col < track_width; ++col)
    {
        const bool is_thumb = col >= thumb_left && col < (thumb_left + thumb_width);
        track.push_back(ftxui::text(is_thumb ? "-" : ".") |
                        ftxui::color(is_thumb ? theme::scrollbar_thumb_fg : theme::scrollbar_track_fg));
    }

    return ftxui::hbox(std::move(track));
}

ftxui::Element LogTextView::render_line(const LogTextViewLine& line, const LogTextViewRenderData& data)
{
    const int first_visible_col = std::max(0, data.first_visible_col);
    const int viewport_cols     = std::max(1, data.viewport_col_count);
    const int line_length       = static_cast<int>(line.text.size());

    std::string visible_text;
    if (first_visible_col < line_length)
    {
        const int visible_length = std::min(line_length - first_visible_col, viewport_cols);
        visible_text             = line.text.substr(static_cast<std::size_t>(first_visible_col), static_cast<std::size_t>(visible_length));
    }

    if (line.styled_ranges.empty())
    {
        auto element = ftxui::text(visible_text);
        if (line.is_find_match)
        {
            element = theme::apply_find_highlight(std::move(element), line.is_active_match);
        }
        return element;
    }

    std::vector<LogTextViewStyledRange> visible_ranges;
    visible_ranges.reserve(line.styled_ranges.size());
    for (const auto& range : line.styled_ranges)
    {
        const int visible_start = std::max(0, range.col_start - first_visible_col);
        const int visible_end = std::min(static_cast<int>(visible_text.size()), std::max(0, range.col_end - first_visible_col));
        if (visible_start >= visible_end)
        {
            continue;
        }

        visible_ranges.push_back(LogTextViewStyledRange {
            visible_start,
            visible_end,
            range.style,
        });
    }

    if (visible_ranges.empty())
    {
        auto element = ftxui::text(visible_text);
        if (line.is_find_match)
        {
            element = theme::apply_find_highlight(std::move(element), line.is_active_match);
        }
        return element;
    }

    std::vector<int> boundaries;
    boundaries.reserve(2 + (visible_ranges.size() * 2));
    boundaries.push_back(0);
    boundaries.push_back(static_cast<int>(visible_text.size()));
    for (const auto& range : visible_ranges)
    {
        boundaries.push_back(range.col_start);
        boundaries.push_back(range.col_end);
    }

    std::sort(boundaries.begin(), boundaries.end());
    boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());

    ftxui::Elements segments;
    segments.reserve(boundaries.size());
    for (std::size_t index = 0; index + 1 < boundaries.size(); ++index)
    {
        const int segment_start = boundaries[index];
        const int segment_end   = boundaries[index + 1];
        if (segment_start >= segment_end)
        {
            continue;
        }

        auto segment = ftxui::text(
            visible_text.substr(static_cast<std::size_t>(segment_start), static_cast<std::size_t>(segment_end - segment_start)));

        const bool preview_active = std::any_of(visible_ranges.begin(), visible_ranges.end(), [&](const auto& range)
                                                { return range_overlaps_segment(range, segment_start, segment_end, LogTextViewRangeStyle::HideColumnsPreview); });
        const bool selection_active = std::any_of(visible_ranges.begin(), visible_ranges.end(), [&](const auto& range)
                                                  { return range_overlaps_segment(range, segment_start, segment_end, LogTextViewRangeStyle::Selection); });

        if (preview_active)
        {
            segment |= ftxui::bgcolor(theme::hide_columns_preview_bg) | ftxui::color(theme::hide_columns_preview_fg);
        }

        if (selection_active)
        {
            segment |= ftxui::inverted;
        }

        segments.push_back(std::move(segment));
    }

    auto element = ftxui::hbox(std::move(segments));
    if (line.is_find_match)
    {
        element = theme::apply_find_highlight(std::move(element), line.is_active_match);
    }
    return element;
}

ftxui::Element LogTextView::render(const LogTextViewRenderData& data)
{
    const int visible_lines = effective_viewport_line_count(data);

    ftxui::Elements rows;
    rows.reserve(static_cast<std::size_t>(std::max(1, visible_lines)));
    if (data.total_lines == 0)
    {
        rows.push_back(ftxui::text(data.empty_text) | ftxui::dim);
    }
    else
    {
        for (const auto& line : data.visible_lines)
        {
            rows.push_back(render_line(line, data));
        }
    }
    rows.push_back(ftxui::filler());

    return ftxui::vbox({
               ftxui::hbox({
                   ftxui::vbox(std::move(rows)) | ftxui::flex | ftxui::reflect(_content_box),
                   render_scrollbar(data),
               }) | ftxui::flex,
               render_hscrollbar(data),
           }) |
           ftxui::reflect(_box);
}

int LogTextView::visible_line_count(int fallback_line_count) const
{
    return measured_line_count(_content_box, fallback_line_count);
}

int LogTextView::visible_col_count() const
{
    return measured_col_count(_content_box);
}

const ftxui::Box& LogTextView::content_box() const
{
    return _content_box;
}

} // namespace slayerlog
