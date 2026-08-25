#include "db/sqlvalue.h"

#include "exceptions.h"

#include <QColor>
#include <QDateTime>
#include <QVariant>
#include <gtest/gtest.h>

TEST(SqlValueTest, DateTimeRoundTrip)
{
  const auto date_time = QDateTime{QDate{2026, 2, 3}, QTime{14, 35, 7}};
  EXPECT_EQ(date_time, date_time_from_sql(to_sql(date_time)));
}

TEST(SqlValueTest, InvalidDateTimeIsNull)
{
  // This round-trip is what expresses "the interval is still running".
  EXPECT_TRUE(to_sql(QDateTime{}).isNull());
  EXPECT_FALSE(date_time_from_sql(QVariant{}).isValid());
}

TEST(SqlValueTest, DateTimeIsFixedWidth)
{
  // Fixed width is what makes a lexicographic BETWEEN on begin_time a correct chronological
  // range scan. A single-digit month or hour would break ordering.
  const auto early = to_sql(QDateTime{QDate{2026, 2, 3}, QTime{4, 5, 6}}).toString();
  const auto late = to_sql(QDateTime{QDate{2026, 11, 30}, QTime{14, 35, 7}}).toString();
  EXPECT_EQ(19, early.length());
  EXPECT_EQ(early.length(), late.length());
  EXPECT_LT(early, late);
}

TEST(SqlValueTest, DateRoundTrip)
{
  const auto date = QDate{2026, 2, 3};
  EXPECT_EQ(date, date_from_sql(to_sql(date)));
  EXPECT_TRUE(to_sql(QDate{}).isNull());
  EXPECT_FALSE(date_from_sql(QVariant{}).isValid());
}

TEST(SqlValueTest, ColorRoundTripKeepsAlpha)
{
  // The previous JSON format used QColor::name(), which drops alpha silently.
  const auto color = QColor{12, 34, 56, 78};
  const auto restored = color_from_sql(to_sql(color));
  EXPECT_EQ(color, restored);
  EXPECT_EQ(78, restored.alpha());
}

TEST(SqlValueTest, ColorWithoutAlphaStillParses)
{
  const auto restored = color_from_sql(QVariant{QStringLiteral("#0c2238")});
  EXPECT_EQ(QColor(12, 34, 56), restored);
}

TEST(SqlValueTest, MinutesRoundTripIncludingNegative)
{
  using namespace std::chrono_literals;
  EXPECT_EQ(0min, minutes_from_sql(to_sql(0min)));
  EXPECT_EQ(485min, minutes_from_sql(to_sql(485min)));
  // Plan::overtime_offset may be negative.
  EXPECT_EQ(-1234min, minutes_from_sql(to_sql(-1234min)));
}

TEST(SqlValueTest, MalformedValuesThrow)
{
  EXPECT_THROW(date_time_from_sql(QVariant{QStringLiteral("not a date")}), DatabaseError);
  EXPECT_THROW(date_from_sql(QVariant{QStringLiteral("not a date")}), DatabaseError);
  EXPECT_THROW(color_from_sql(QVariant{QStringLiteral("not a color")}), DatabaseError);
  EXPECT_THROW(minutes_from_sql(QVariant{QStringLiteral("not a number")}), DatabaseError);
}
