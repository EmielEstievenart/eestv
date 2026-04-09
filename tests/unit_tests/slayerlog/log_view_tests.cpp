#include <gtest/gtest.h>

#include <optional>

#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

#include "log_controller.hpp"
#include "log_model.hpp"
#include "log_text_view.hpp"
#include "log_view.hpp"

namespace slayerlog
{

namespace
{

std::string strip_ansi_sequences(const std::string& text)
{
    std::string result;
    result.reserve(text.size());

    std::size_t index = 0;
    while (index < text.size())
    {
        if (text[index] != '\x1b')
        {
            result.push_back(text[index]);
            ++index;
            continue;
        }

        ++index;
        if (index >= text.size() || text[index] != '[')
        {
            continue;
        }

        ++index;
        while (index < text.size() && (text[index] < '@' || text[index] > '~'))
        {
            ++index;
        }
        if (index < text.size())
        {
            ++index;
        }
    }

    return result;
}

void render_view(ftxui::Element document, int width, int height)
{
    auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(width), ftxui::Dimension::Fixed(height));
    Render(screen, document);
}

} // namespace

TEST(LogTextViewTest, RenderShowsHorizontalSliceAndScrollbar)
{
    LogTextView text_view;
    LogTextViewRenderData data;
    data.total_lines         = 1;
    data.first_visible_line  = 0;
    data.viewport_line_count = 1;
    data.first_visible_col   = 3;
    data.max_line_width      = 10;
    data.viewport_col_count  = 4;
    data.visible_lines.push_back(LogTextViewLine {
        "0123456789",
        {},
        false,
        false,
    });

    auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(4), ftxui::Dimension::Fixed(2));
    Render(screen, text_view.render(data) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 4) | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 2));

    EXPECT_EQ(strip_ansi_sequences(screen.ToString()), "3456\r\n.-..");
}

TEST(LogViewTest, MouseMappingAccountsForHorizontalOffset)
{
    LogModel model;
    LogController controller;
    LogView view;

    model.append_lines({
        ObservedLogLine {"alpha.log", "abcdefghijklmnopqrstuvwxyz0123456789"},
    });

    render_view(view.render(model, controller, "header", 10), 20, 10);
    render_view(view.render(model, controller, "header", 10), 20, 10);

    controller.scroll_right(model, 4);
    render_view(view.render(model, controller, "header", 10), 20, 10);

    std::optional<TextPosition> first_position;
    for (int y = 0; y < 10 && !first_position.has_value(); ++y)
    {
        for (int x = 0; x < 20; ++x)
        {
            ftxui::Mouse mouse;
            mouse.x = x;
            mouse.y = y;

            const auto position = view.mouse_to_text_position(model, controller, mouse);
            if (position.has_value())
            {
                first_position = position;
                break;
            }
        }
    }

    ASSERT_TRUE(first_position.has_value());
    EXPECT_EQ(first_position->line, 0);
    EXPECT_EQ(first_position->column, controller.first_visible_col(model));
}

} // namespace slayerlog
