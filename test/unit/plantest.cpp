#include "plan.h"

#include <gtest/gtest.h>

TEST(PlanTest, KindsIn)
{
  FullTimePlan plan;
  const auto add_holiday = [&plan](const QDate& begin, const QDate& end) {
    plan.add(std::make_unique<Plan::Entry>(Period{begin, end}, Plan::Kind::Holiday));
  };

  add_holiday(QDate{2025, 1, 1}, QDate{2025, 1, 2});
  add_holiday(QDate{2025, 1, 3}, QDate{2025, 1, 6});
  add_holiday(QDate{2025, 1, 10}, QDate{2025, 1, 10});

  using enum Plan::Kind;
  EXPECT_EQ((std::vector{Holiday, Holiday, Holiday, Holiday, Holiday, Holiday, Normal, Normal, Normal, Holiday}),
            plan.kinds_in(Period{QDate{2025, 1, 1}, QDate{2025, 1, 10}}));
  EXPECT_EQ((std::vector{Normal, Normal, Holiday, Holiday, Holiday, Holiday, Holiday, Holiday, Normal, Normal, Normal,
                         Holiday, Normal, Normal}),
            plan.kinds_in(Period{QDate{2024, 12, 30}, QDate{2025, 1, 12}}));
  EXPECT_EQ((std::vector<Plan::Kind>{}), plan.kinds_in(Period{}));
  EXPECT_EQ((std::vector{Normal, Normal, Normal}), plan.kinds_in(Period{QDate{2025, 6, 8}, QDate{2025, 6, 10}}));
  EXPECT_EQ((std::vector{Normal}), plan.kinds_in(Period{QDate{2025, 1, 7}, QDate{2025, 1, 7}}));

  const FullTimePlan empty_plan;
  EXPECT_EQ((std::vector(10, Normal)), empty_plan.kinds_in(Period{QDate{2025, 1, 1}, QDate{2025, 1, 10}}));
  EXPECT_EQ((std::vector(14, Normal)), empty_plan.kinds_in(Period{QDate{2024, 12, 30}, QDate{2025, 1, 12}}));
}

std::ostream& operator<<(std::ostream& o, const Plan::Kind kind)
{
  const auto name = [kind] {
    switch (kind) {
      using enum Plan::Kind;
    case Holiday:
      return "Holiday";
    case Normal:
      return "Normal";
    case Sick:
      return "Sick";
    case HalfHoliday:
      return "HalfHoliday";
    case Vacation:
      return "Vacation";
    case HalfVacation:
      return "HalfVacation";
    case HalfVacationHalfHoliday:
      return "HalfVacationHalfHoliday";
    }
    Q_UNREACHABLE();
  }();
  return o << name;
}

TEST(PlanTest, add_sorted)
{
  std::vector<std::unique_ptr<Plan::Entry>> periods;
  static constexpr auto make_entry = [](const QDate& begin, const QDate& end) {
    return std::make_unique<Plan::Entry>(Period{begin, end}, Plan::Kind::Holiday);
  };
  const auto add_period = [&periods](const QDate& begin, const QDate& end) {
    auto entry = make_entry(begin, end);
    if (const auto it = find_period_insert_pos(periods, entry->period); it.has_value()) {
      const auto i = std::distance(periods.cbegin(), *it);
      periods.insert(*it, std::move(entry));
      return static_cast<int>(i);
    }
    return -1;
  };

  // add a period to an empty range
  ASSERT_EQ(0, add_period(QDate{2025, 2, 1}, QDate{2025, 2, 3}));

  // add the same period again is expected to fail
  ASSERT_EQ(-1, add_period(QDate{2025, 2, 1}, QDate{2025, 2, 3}));

  // add a period which overlaps (2025/2/1)
  ASSERT_EQ(-1, add_period(QDate{2025, 1, 1}, QDate{2025, 2, 1}));

  // add a period which overlaps (2025/2/3)
  ASSERT_EQ(-1, add_period(QDate{2025, 2, 3}, QDate{2025, 2, 4}));

  // add a period which doesn't overlap
  ASSERT_EQ(1, add_period(QDate{2025, 3, 3}, QDate{2025, 3, 4}));

  // add a period which doesn't overlap in between the two existing ones
  ASSERT_EQ(1, add_period(QDate{2025, 2, 10}, QDate{2025, 2, 12}));

  // add a period which overlap the two existing ones is expected to fail
  ASSERT_EQ(-1, add_period(QDate{2025, 1, 14}, QDate{2025, 3, 3}));
}
