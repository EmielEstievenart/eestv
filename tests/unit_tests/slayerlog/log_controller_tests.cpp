#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include <ftxui/component/event.hpp>

#include "log_controller.hpp"
#include "log_model.hpp"

namespace slayerlog
{

namespace
{

std::vector<ObservedLogLine> numbered_lines(int count)
{
    std::vector<ObservedLogLine> lines;
    lines.reserve(static_cast<std::size_t>(count));
    for (int index = 1; index <= count; ++index)
    {
        lines.push_back({"alpha.log", "line " + std::to_string(index)});
    }

    return lines;
}

} // namespace

TEST(LogControllerTest, ScrollStateTracksViewportAndFollowBottom)
{
    LogModel model;
    LogController controller;
    model.append_lines(numbered_lines(10));

    EXPECT_EQ(controller.first_visible_line_index(model, 3).value, 7);

    controller.scroll_up(model, 3, 2);
    EXPECT_EQ(controller.first_visible_line_index(model, 3).value, 5);

    model.append_lines(numbered_lines(2));
    EXPECT_EQ(controller.first_visible_line_index(model, 3).value, 5);

    controller.scroll_to_bottom();
    EXPECT_EQ(controller.first_visible_line_index(model, 3).value, 9);
}

TEST(LogControllerTest, GoToLineCentersVisibleContentAndFailsForHiddenLine)
{
    LogModel model;
    LogController controller;
    model.append_lines({
        ObservedLogLine {"alpha.log", "info one"},
        ObservedLogLine {"alpha.log", "error two"},
        ObservedLogLine {"alpha.log", "info three"},
        ObservedLogLine {"alpha.log", "info four"},
        ObservedLogLine {"alpha.log", "error five"},
    });
    model.add_include_filter("error");

    EXPECT_TRUE(controller.go_to_line(model, 5, 1));
    EXPECT_EQ(controller.first_visible_line_index(model, 1).value, 1);

    EXPECT_FALSE(controller.go_to_line(model, 1, 1));
}

TEST(LogControllerTest, FindNavigationUsesVisibleMatchesAndWraps)
{
    LogModel model;
    LogController controller;
    model.append_lines({
        ObservedLogLine {"alpha.log", "error first"},
        ObservedLogLine {"alpha.log", "error hidden"},
        ObservedLogLine {"alpha.log", "error third"},
    });
    model.add_exclude_filter("hidden");

    ASSERT_TRUE(controller.set_find_query(model, "error", 1));
    ASSERT_TRUE(controller.active_find_visible_index(model).has_value());
    EXPECT_EQ(controller.active_find_visible_index(model)->value, 0);
    EXPECT_EQ(controller.first_visible_line_index(model, 1).value, 0);

    EXPECT_TRUE(controller.go_to_next_find_match(model, 1));
    ASSERT_TRUE(controller.active_find_visible_index(model).has_value());
    EXPECT_EQ(controller.active_find_visible_index(model)->value, 1);
    EXPECT_EQ(controller.first_visible_line_index(model, 1).value, 1);

    EXPECT_TRUE(controller.go_to_next_find_match(model, 1));
    ASSERT_TRUE(controller.active_find_visible_index(model).has_value());
    EXPECT_EQ(controller.active_find_visible_index(model)->value, 0);
}

TEST(LogControllerTest, FindNavigationRecoversWhenActiveMatchBecomesHidden)
{
    LogModel model;
    LogController controller;
    model.append_lines({
        ObservedLogLine {"alpha.log", "error one"},
        ObservedLogLine {"alpha.log", "error two"},
        ObservedLogLine {"alpha.log", "error three"},
    });

    ASSERT_TRUE(controller.set_find_query(model, "error", 1));
    ASSERT_TRUE(controller.go_to_next_find_match(model, 1));
    ASSERT_TRUE(controller.active_find_visible_index(model).has_value());
    EXPECT_EQ(controller.active_find_visible_index(model)->value, 1);

    model.add_include_filter("three");
    EXPECT_FALSE(controller.active_find_visible_index(model).has_value());

    EXPECT_TRUE(controller.go_to_next_find_match(model, 1));
    ASSERT_TRUE(controller.active_find_visible_index(model).has_value());
    EXPECT_EQ(controller.active_find_visible_index(model)->value, 0);
    EXPECT_EQ(model.rendered_line(controller.active_find_visible_index(model)->value), "3 error three");
}

TEST(LogControllerTest, InvalidRegexFindKeepsExistingFindState)
{
    LogModel model;
    LogController controller;
    model.append_lines({
        ObservedLogLine {"alpha.log", "error one"},
        ObservedLogLine {"alpha.log", "error two"},
        ObservedLogLine {"alpha.log", "info three"},
    });

    ASSERT_TRUE(controller.set_find_query(model, "error", 1));
    ASSERT_TRUE(controller.active_find_visible_index(model).has_value());
    const auto active_before = controller.active_find_visible_index(model);

    EXPECT_THROW(controller.set_find_query(model, "re:[", 1), std::invalid_argument);

    EXPECT_TRUE(model.find_active());
    EXPECT_EQ(model.find_query(), "error");
    EXPECT_EQ(model.total_find_match_count(), 2);
    EXPECT_EQ(controller.active_find_visible_index(model), active_before);
}

TEST(LogControllerTest, HandleEventEscapeClearsFindBeforeRequestingExit)
{
    LogModel model;
    LogController controller;
    model.append_lines({
        ObservedLogLine {"alpha.log", "error one"},
        ObservedLogLine {"alpha.log", "error two"},
    });
    ASSERT_TRUE(controller.set_find_query(model, "error", 1));
    ASSERT_TRUE(model.find_active());

    const auto result = controller.handle_event(model, ftxui::Event::Escape, 1, {});

    EXPECT_TRUE(result.handled);
    EXPECT_FALSE(result.request_exit);
    EXPECT_FALSE(model.find_active());
    EXPECT_EQ(model.total_find_match_count(), 0);
}

TEST(LogControllerTest, HandleEventLeftAndRightArrowNavigateFindResults)
{
    LogModel model;
    LogController controller;
    model.append_lines({
        ObservedLogLine {"alpha.log", "error one"},
        ObservedLogLine {"alpha.log", "error two"},
        ObservedLogLine {"alpha.log", "error three"},
    });
    ASSERT_TRUE(controller.set_find_query(model, "error", 1));
    ASSERT_TRUE(controller.go_to_next_find_match(model, 1));
    ASSERT_TRUE(controller.active_find_visible_index(model).has_value());
    EXPECT_EQ(controller.active_find_visible_index(model)->value, 1);

    const auto previous_result = controller.handle_event(model, ftxui::Event::ArrowLeft, 1, {});
    EXPECT_TRUE(previous_result.handled);
    EXPECT_FALSE(previous_result.request_exit);
    ASSERT_TRUE(controller.active_find_visible_index(model).has_value());
    EXPECT_EQ(controller.active_find_visible_index(model)->value, 0);

    const auto next_result = controller.handle_event(model, ftxui::Event::ArrowRight, 1, {});
    EXPECT_TRUE(next_result.handled);
    EXPECT_FALSE(next_result.request_exit);
    ASSERT_TRUE(controller.active_find_visible_index(model).has_value());
    EXPECT_EQ(controller.active_find_visible_index(model)->value, 1);
}

TEST(LogControllerTest, HorizontalScrollUsesArrowKeysWhenFindInactive)
{
    LogModel model;
    LogController controller;
    model.append_lines({
        ObservedLogLine {"alpha.log", "abcdefghij"},
    });

    controller.update_viewport_col_count(5);

    const auto right_result = controller.handle_event(model, ftxui::Event::ArrowRight, 1, {});
    EXPECT_TRUE(right_result.handled);
    EXPECT_FALSE(right_result.request_exit);
    EXPECT_EQ(controller.first_visible_col(model), 1);

    const auto left_result = controller.handle_event(model, ftxui::Event::ArrowLeft, 1, {});
    EXPECT_TRUE(left_result.handled);
    EXPECT_FALSE(left_result.request_exit);
    EXPECT_EQ(controller.first_visible_col(model), 0);
}

TEST(LogControllerTest, HorizontalScrollClampsToRenderedWidth)
{
    LogModel model;
    LogController controller;
    model.append_lines({
        ObservedLogLine {"alpha.log", "abcdefghij"},
    });

    controller.update_viewport_col_count(4);
    controller.scroll_right(model, 100);

    EXPECT_EQ(controller.first_visible_col(model), model.max_rendered_line_width() - 4);

    controller.scroll_left(model, 100);
    EXPECT_EQ(controller.first_visible_col(model), 0);
}

TEST(LogControllerTest, SelectionTracksBoundsAndExtractsText)
{
    LogModel model;
    LogController controller;
    model.append_lines({
        ObservedLogLine {"alpha.log", "alpha"},
        ObservedLogLine {"alpha.log", "bravo"},
    });

    const auto first  = model.rendered_line(0);
    const auto second = model.rendered_line(1);

    controller.begin_selection(model, TextPosition {0, 2});
    EXPECT_TRUE(controller.selection_in_progress());
    controller.update_selection(model, TextPosition {1, 3});
    controller.end_selection(model, TextPosition {1, 3});

    EXPECT_FALSE(controller.selection_in_progress());
    ASSERT_TRUE(controller.selection_bounds(model).has_value());
    const auto bounds = *controller.selection_bounds(model);
    EXPECT_EQ(bounds.first.line, 0);
    EXPECT_EQ(bounds.first.column, 2);
    EXPECT_EQ(bounds.second.line, 1);
    EXPECT_EQ(bounds.second.column, 3);

    EXPECT_EQ(controller.selection_text(model), first.substr(2) + "\n" + second.substr(0, 3));

    controller.clear_selection();
    EXPECT_FALSE(controller.selection_bounds(model).has_value());
    EXPECT_TRUE(controller.selection_text(model).empty());
}

TEST(LogControllerTest, RenderDataBuildsPreviewAndSelectionRanges)
{
    LogModel model;
    LogController controller;
    model.append_lines({
        ObservedLogLine {"alpha.log", "abcdefghij"},
    });

    controller.update_viewport_col_count(5);
    controller.scroll_right(model, 3);

    const auto preview_data = controller.render_data(model, 1, HiddenColumnRange {3, 5});
    ASSERT_EQ(preview_data.visible_lines.size(), 1U);
    ASSERT_EQ(preview_data.visible_lines[0].styled_ranges.size(), 1U);
    EXPECT_EQ(preview_data.first_visible_col, 3);
    EXPECT_EQ(preview_data.visible_lines[0].styled_ranges[0].style, LogTextViewRangeStyle::HideColumnsPreview);
    EXPECT_EQ(preview_data.visible_lines[0].styled_ranges[0].col_start, 4);
    EXPECT_EQ(preview_data.visible_lines[0].styled_ranges[0].col_end, 7);

    controller.begin_selection(model, TextPosition {0, 5});
    controller.update_selection(model, TextPosition {0, 8});
    controller.end_selection(model, TextPosition {0, 8});

    const auto selection_data = controller.render_data(model, 1, HiddenColumnRange {3, 5});
    ASSERT_EQ(selection_data.visible_lines.size(), 1U);
    ASSERT_EQ(selection_data.visible_lines[0].styled_ranges.size(), 1U);
    EXPECT_EQ(selection_data.visible_lines[0].styled_ranges[0].style, LogTextViewRangeStyle::Selection);
    EXPECT_EQ(selection_data.visible_lines[0].styled_ranges[0].col_start, 5);
    EXPECT_EQ(selection_data.visible_lines[0].styled_ranges[0].col_end, 8);
}

TEST(LogControllerTest, ResetClearsControllerState)
{
    LogModel model;
    LogController controller;
    model.append_lines(numbered_lines(6));

    controller.scroll_to_top(model, 2);
    EXPECT_EQ(controller.first_visible_line_index(model, 2).value, 0);

    ASSERT_TRUE(controller.set_find_query(model, "line", 2));
    ASSERT_TRUE(controller.active_find_visible_index(model).has_value());

    controller.update_viewport_col_count(4);
    controller.scroll_right(model, 2);

    controller.begin_selection(model, TextPosition {0, 1});
    controller.update_selection(model, TextPosition {1, 2});
    EXPECT_TRUE(controller.selection_in_progress());

    controller.reset();

    EXPECT_EQ(controller.first_visible_line_index(model, 2).value, 4);
    EXPECT_EQ(controller.first_visible_col(model), 0);
    EXPECT_FALSE(controller.active_find_visible_index(model).has_value());
    EXPECT_FALSE(controller.selection_in_progress());
    EXPECT_FALSE(controller.selection_bounds(model).has_value());
    EXPECT_TRUE(controller.selection_text(model).empty());
}

} // namespace slayerlog
