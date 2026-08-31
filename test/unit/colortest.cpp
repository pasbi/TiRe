#include "colorutil.h"
#include "fmt.h"

#include <QDate>
#include <gtest/gtest.h>
#include <ranges>
#include <spdlog/spdlog.h>

[[nodiscard]] bool operator<(const QColor& a, const QColor& b) noexcept
{
  static constexpr auto to_tuple = [](const QColor& color) {
    return std::tuple(color.red(), color.green(), color.blue(), color.alpha());
  };
  return to_tuple(a) < to_tuple(b);
}

void PrintTo(const QColor& color, std::ostream* const os)
{
  *os << color.name().toStdString();
}

TEST(ColorTest, background)
{
  std::set<QColor> colors_weekend;
  std::set<QColor> colors_week;

  const auto base = QDate{2024, 1, 1};
  for (const auto offset : std::views::iota(0, 365)) {
    const auto date = base.addDays(offset);
    const auto color = ::background(date);
    if (date.dayOfWeek() == Qt::Sunday || date.dayOfWeek() == Qt::Saturday) {
      colors_weekend.insert(color);
    } else {
      colors_week.insert(color);
    }
  }

  ASSERT_EQ(colors_weekend.size(), 1);
  ASSERT_EQ(colors_week.size(), 1);

  const auto color_week = *colors_week.begin();
  const auto color_weekend = *colors_weekend.begin();

  // TODO Use Lab colorspace to check if the colors are distinctive enough.
  // TODO These tests depends on QPalette, which depends on the test environment.
  EXPECT_NE(color_week, color_weekend);
  EXPECT_NE(color_week, ::selected(color_week));
  EXPECT_NE(color_week, ::selected(color_weekend));
  EXPECT_NE(color_weekend, ::selected(color_week));
  EXPECT_NE(color_weekend, ::selected(color_weekend));
  EXPECT_NE(::selected(color_week), ::selected(color_weekend));
}
