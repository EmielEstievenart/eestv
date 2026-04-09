#pragma once

#include <string>
#include <vector>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

namespace slayerlog
{

enum class LogTextViewRangeStyle
{
    HideColumnsPreview,
    Selection,
};

struct LogTextViewStyledRange
{
    int col_start = 0;
    int col_end   = 0;
    LogTextViewRangeStyle style = LogTextViewRangeStyle::HideColumnsPreview;
};

struct LogTextViewLine
{
    std::string text;
    std::vector<LogTextViewStyledRange> styled_ranges;
    bool is_find_match   = false;
    bool is_active_match = false;
};

struct LogTextViewRenderData
{
    int total_lines         = 0;
    int first_visible_line  = 0;
    int viewport_line_count = 1;
    int first_visible_col   = 0;
    int max_line_width      = 0;
    int viewport_col_count  = 1;
    std::string empty_text  = "<empty>";
    std::vector<LogTextViewLine> visible_lines;
};

class LogTextView
{
public:
    [[nodiscard]] ftxui::Element render(const LogTextViewRenderData& data);
    [[nodiscard]] int visible_line_count(int fallback_line_count) const;
    [[nodiscard]] int visible_col_count() const;
    [[nodiscard]] const ftxui::Box& content_box() const;

private:
    ftxui::Box _box;
    ftxui::Box _content_box;

    [[nodiscard]] static ftxui::Element render_scrollbar(const LogTextViewRenderData& data);
    [[nodiscard]] static ftxui::Element render_hscrollbar(const LogTextViewRenderData& data);
    [[nodiscard]] static ftxui::Element render_line(const LogTextViewLine& line, const LogTextViewRenderData& data);
};

} // namespace slayerlog
