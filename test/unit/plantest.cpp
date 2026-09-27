#include "plan.h"

#include "application.h"
#include "commands/commands.h"
#include "commands/undostack.h"
#include "exceptions.h"
#include "interval.h"
#include "intervalmodel.h"
#include "project.h"

#include <QDateTime>
#include <gtest/gtest.h>

namespace
{

[[nodiscard]] std::unique_ptr<Plan::Entry> make_entry(const QDate& begin, const QDate& end, const Plan::Kind kind,
                                                      const Plan::Share share)
{
  return std::make_unique<Plan::Entry>(Period{begin, end}, kind, share, EntityId{});
}

[[nodiscard]] Plan::Share half()
{
  return Plan::Share::from_percent(50);
}

}  // namespace

std::ostream& operator<<(std::ostream& o, const Plan::Kind kind)
{
  return o << fmt::format("{}", kind);
}

std::ostream& operator<<(std::ostream& o, const Plan::Share share)
{
  return o << fmt::format("{}", share);
}

TEST(PlanTest, SharesIn)
{
  FullTimePlan plan;
  const auto add_holiday = [&plan](const QDate& begin, const QDate& end) {
    plan.add(::make_entry(begin, end, Plan::Kind::Holiday, Plan::Share::whole()));
  };

  add_holiday(QDate{2025, 1, 1}, QDate{2025, 1, 2});
  add_holiday(QDate{2025, 1, 3}, QDate{2025, 1, 6});
  add_holiday(QDate{2025, 1, 10}, QDate{2025, 1, 10});

  const auto holiday_shares = [&plan](const Period& period) {
    std::vector<Plan::Share> shares;
    for (const auto& day : plan.shares_in(period)) {
      shares.push_back(day[Plan::Kind::Holiday]);
    }
    return shares;
  };

  const auto whole = Plan::Share::whole();
  const auto none = Plan::Share::none();
  EXPECT_EQ((std::vector{whole, whole, whole, whole, whole, whole, none, none, none, whole}),
            holiday_shares(Period{QDate{2025, 1, 1}, QDate{2025, 1, 10}}));
  EXPECT_EQ((std::vector{none, none, whole, whole, whole, whole, whole, whole, none, none, none, whole, none, none}),
            holiday_shares(Period{QDate{2024, 12, 30}, QDate{2025, 1, 12}}));
  EXPECT_TRUE(plan.shares_in(Period{}).empty()) << "an invalid period has no days at all";
  EXPECT_EQ((std::vector{none, none, none}), holiday_shares(Period{QDate{2025, 6, 8}, QDate{2025, 6, 10}}));
  EXPECT_EQ((std::vector{none}), holiday_shares(Period{QDate{2025, 1, 7}, QDate{2025, 1, 7}}));

  const FullTimePlan empty_plan;
  EXPECT_EQ(10U, empty_plan.shares_in(Period{QDate{2025, 1, 1}, QDate{2025, 1, 10}}).size());
  EXPECT_TRUE(empty_plan.shares_in(Period{QDate{2025, 1, 1}, QDate{2025, 1, 10}}).front().empty());
}

TEST(PlanTest, overlapping_entries_share_a_day)
{
  // The case the old combined HALF_VACATION_HALF_HOLIDAY kind existed for, now expressed as what
  // it always was: two bookings that happen to meet on one day.
  FullTimePlan plan;
  const auto christmas_eve = QDate{2026, 12, 24};
  ASSERT_TRUE(plan.add(::make_entry(christmas_eve, christmas_eve, Plan::Kind::Holiday, ::half())));
  ASSERT_TRUE(plan.add(::make_entry(christmas_eve, christmas_eve, Plan::Kind::Vacation, ::half())));

  const auto shares = plan.shares_on(christmas_eve);
  EXPECT_EQ(::half(), shares[Plan::Kind::Holiday]);
  EXPECT_EQ(::half(), shares[Plan::Kind::Vacation]);
  EXPECT_EQ(Plan::Share::whole(), shares.budget()) << "the day is fully covered";

  using std::chrono_literals::operator""h;
  using std::chrono_literals::operator""min;
  const IntervalModel interval_model;
  EXPECT_EQ(0min, plan.planned_working_time(Period{christmas_eve, christmas_eve}, interval_model))
      << "half a bank holiday topped up with half a vacation day is a whole day off";
  // Only half a vacation day is spent on it, which is the whole point of the combination.
  EXPECT_EQ(4h, plan.leave_time(Plan::Kind::Vacation, Period{christmas_eve, christmas_eve}, interval_model));
  EXPECT_EQ(4h, plan.leave_time(Plan::Kind::Holiday, Period{christmas_eve, christmas_eve}, interval_model));
}

TEST(PlanTest, a_day_cannot_hold_more_leave_than_it_is_long)
{
  FullTimePlan plan;
  const auto day = QDate{2026, 4, 6};
  ASSERT_TRUE(plan.add(::make_entry(day, day, Plan::Kind::Vacation, ::half())));

  using enum Plan::Conflict;
  EXPECT_EQ(None, plan.check_add(*::make_entry(day, day, Plan::Kind::Holiday, ::half())));
  EXPECT_EQ(Overbooked, plan.check_add(*::make_entry(day, day, Plan::Kind::Holiday, Plan::Share::whole())));
  // A share nobody could honor is refused before it is compared to anything.
  EXPECT_EQ(ImpossibleShare, plan.check_add(*::make_entry(day, day, Plan::Kind::Holiday, Plan::Share::none())));
  EXPECT_EQ(ImpossibleShare,
            plan.check_add(*::make_entry(day, day, Plan::Kind::Holiday, Plan::Share::from_percent(150))));

  // add() refuses what check_add() refuses, rather than storing a day that cannot be accounted for.
  EXPECT_FALSE(plan.add(::make_entry(day, day, Plan::Kind::Holiday, Plan::Share::whole())));
  EXPECT_EQ(1, plan.rowCount({}));
}

TEST(PlanTest, own_and_child_sickness_cannot_share_a_day)
{
  // Both waivers only cover the hours actually missed, so a day carrying both could not say which
  // allowance those hours came out of -- and the child one is capped by law.
  FullTimePlan plan;
  const auto day = QDate{2026, 4, 7};
  ASSERT_TRUE(plan.add(::make_entry(day, day, Plan::Kind::Sick, ::half())));

  using enum Plan::Conflict;
  EXPECT_EQ(MixedSickness, plan.check_add(*::make_entry(day, day, Plan::Kind::ChildSick, ::half())));
  // The same amount of leave is fine as long as it draws on a budget instead.
  EXPECT_EQ(None, plan.check_add(*::make_entry(day, day, Plan::Kind::Vacation, ::half())));
}

TEST(PlanTest, check_period)
{
  FullTimePlan plan;
  const auto add = [&plan](const QDate& begin, const QDate& end) {
    plan.add(::make_entry(begin, end, Plan::Kind::Holiday, Plan::Share::whole()));
  };
  add(QDate{2025, 2, 1}, QDate{2025, 2, 3});
  add(QDate{2025, 3, 1}, QDate{2025, 3, 4});

  using enum Plan::Conflict;
  // Keeping a period unchanged must be allowed: the entry may of course overlap itself.
  EXPECT_EQ(None, plan.check_period(plan.entry(0), Period{QDate{2025, 2, 1}, QDate{2025, 2, 3}}));
  EXPECT_EQ(None, plan.check_period(plan.entry(0), Period{QDate{2025, 1, 1}, QDate{2025, 1, 5}}));
  // Touching the other entry is no longer a conflict; only sharing a day with it is, and only
  // because both claim the whole of it.
  EXPECT_EQ(None, plan.check_period(plan.entry(0), Period{QDate{2025, 2, 1}, QDate{2025, 2, 28}}));
  EXPECT_EQ(Overbooked, plan.check_period(plan.entry(0), Period{QDate{2025, 2, 1}, QDate{2025, 3, 1}}));

  // check_period must agree with what swap_period actually does.
  EXPECT_THROW(plan.swap_period(plan.entry(0), Period{QDate{2025, 2, 1}, QDate{2025, 3, 1}}), RuntimeError);
  EXPECT_NO_THROW(plan.swap_period(plan.entry(0), Period{QDate{2025, 2, 1}, QDate{2025, 2, 28}}));
}

TEST(PlanTest, check_kind_and_share)
{
  FullTimePlan plan;
  const auto day = QDate{2026, 4, 8};
  ASSERT_TRUE(plan.add(::make_entry(day, day, Plan::Kind::Sick, ::half())));
  ASSERT_TRUE(plan.add(::make_entry(day, day, Plan::Kind::Vacation, ::half())));

  using enum Plan::Conflict;
  const auto& vacation = plan.entry(1);
  EXPECT_EQ(MixedSickness, plan.check_kind(vacation, Plan::Kind::ChildSick))
      << "the other half of the day is already own sickness";
  EXPECT_EQ(None, plan.check_kind(vacation, Plan::Kind::Holiday));
  EXPECT_EQ(Overbooked, plan.check_share(vacation, Plan::Share::whole()));
  EXPECT_EQ(None, plan.check_share(vacation, Plan::Share::from_percent(25)));

  EXPECT_THROW(plan.swap_kind(vacation, Plan::Kind::ChildSick), RuntimeError);
  EXPECT_THROW(plan.swap_share(vacation, Plan::Share::whole()), RuntimeError);
  EXPECT_NO_THROW(plan.swap_share(plan.entry(1), Plan::Share::from_percent(25)));
  EXPECT_EQ(Plan::Share::from_percent(25), plan.entry(1).share);
}

TEST(PlanTest, undo_period_change_that_reorders)
{
  // Changing a period re-sorts the entries, so a command must not identify its entry by row
  // index: after the reorder that index refers to a different entry, and undo would rewrite the
  // wrong one.
  FullTimePlan plan;
  const auto add = [&plan](const QDate& date) {
    plan.add(::make_entry(date, date, Plan::Kind::Holiday, Plan::Share::whole()));
  };
  add(QDate{2026, 1, 5});
  add(QDate{2026, 3, 5});

  // Identify the entries by address: Plan owns them through unique_ptrs, so sorting moves the
  // pointers around but never the Entry objects themselves.
  const auto& moved = plan.entry(0);
  const auto& other = plan.entry(1);

  // Move the first entry past the second, which reorders them.
  Application::undo_stack().push(
      make_modify_plan_period_command(plan, moved, Period{QDate{2026, 6, 5}, QDate{2026, 6, 5}}));
  ASSERT_EQ(&other, &plan.entry(0)) << "the entries should have swapped places";
  ASSERT_EQ(&moved, &plan.entry(1));

  Application::undo_stack().undo();

  ASSERT_EQ(2, plan.rowCount({}));
  EXPECT_EQ(Period(QDate(2026, 1, 5), QDate(2026, 1, 5)), moved.period) << "undo must restore the moved entry";
  EXPECT_EQ(Period(QDate(2026, 3, 5), QDate(2026, 3, 5)), other.period) << "undo must not touch the other entry";
  EXPECT_EQ(&moved, &plan.entry(0)) << "and the original order should be back";
}

TEST(PlanTest, delete_entries_is_undoable)
{
  FullTimePlan plan;
  const auto add = [&plan](const QDate& date) {
    plan.add(::make_entry(date, date, Plan::Kind::Holiday, Plan::Share::whole()));
  };
  add(QDate{2026, 1, 5});
  add(QDate{2026, 3, 5});
  const auto& deleted = plan.entry(0);
  const auto& kept = plan.entry(1);

  const auto undo_count = Application::undo_stack().impl().count();
  delete_plan_entries(plan, {});
  EXPECT_EQ(undo_count, Application::undo_stack().impl().count()) << "deleting nothing must not add an undo step";

  delete_plan_entries(plan, {&deleted});
  ASSERT_EQ(1, plan.rowCount({}));
  EXPECT_EQ(&kept, &plan.entry(0)) << "the other entry must survive";

  Application::undo_stack().undo();
  ASSERT_EQ(2, plan.rowCount({}));
  EXPECT_EQ(&deleted, &plan.entry(0)) << "undo must put the very same entry back, in its old place";
  EXPECT_EQ(Period(QDate(2026, 1, 5), QDate(2026, 1, 5)), plan.entry(0).period);
}

TEST(PlanTest, add_sorted)
{
  // Entries are kept in order of their beginning. Overlapping ones are allowed now, so the only
  // thing to check is where each lands.
  FullTimePlan plan;
  const auto add = [&plan](const QDate& begin, const QDate& end) {
    return plan.add(::make_entry(begin, end, Plan::Kind::Vacation, ::half()));
  };
  ASSERT_TRUE(add(QDate{2025, 3, 3}, QDate{2025, 3, 4}));
  ASSERT_TRUE(add(QDate{2025, 2, 1}, QDate{2025, 2, 3}));
  ASSERT_TRUE(add(QDate{2025, 2, 10}, QDate{2025, 2, 12}));
  // Overlapping the first two entries, which is fine as long as no day is overbooked.
  ASSERT_TRUE(add(QDate{2025, 1, 14}, QDate{2025, 3, 3}));

  EXPECT_EQ(QDate(2025, 1, 14), plan.entry(0).period.begin());
  EXPECT_EQ(QDate(2025, 2, 1), plan.entry(1).period.begin());
  EXPECT_EQ(QDate(2025, 2, 10), plan.entry(2).period.begin());
  EXPECT_EQ(QDate(2025, 3, 3), plan.entry(3).period.begin());
  // The day both the long entry and the last one cover holds half of each.
  EXPECT_EQ(Plan::Share::whole(), plan.shares_on(QDate{2025, 3, 3})[Plan::Kind::Vacation]);
}

namespace
{

// 2026-09-21 is a Monday, so a FullTimePlan expects eight hours of work on it; 2026-09-26 is a
// Saturday, on which it expects none.
const auto monday = QDate{2026, 9, 21};
const auto saturday = QDate{2026, 9, 26};

/** @brief A single day of one kind of leave, with a given amount of work booked onto it. */
class PlannedDay
{
public:
  explicit PlannedDay(const QDate& date, const Plan::Kind kind, const Plan::Share share,
                      const std::chrono::minutes worked)
    : m_date(date)
  {
    m_plan.add(::make_entry(m_date, m_date, kind, share));
    using std::chrono_literals::operator""min;
    if (worked > 0min) {
      auto interval = std::make_unique<Interval>(&m_project);
      interval->swap_begin(QDateTime{m_date, QTime{8, 0}});
      interval->swap_end(QDateTime{m_date, QTime{8, 0}}.addSecs(worked.count() * 60));
      m_interval_model.add(std::move(interval));
    }
  }

  /** @brief The target for the day. The day's overtime contribution is the work booked minus this. */
  [[nodiscard]] std::chrono::minutes planned_time() const
  {
    return m_plan.planned_working_time(period(), m_interval_model);
  }
  [[nodiscard]] std::chrono::minutes leave_time(const Plan::Kind kind) const
  {
    return m_plan.leave_time(kind, period(), m_interval_model);
  }

private:
  QDate m_date;
  // Declared before the models so that it outlives the interval pointing at it.
  Project m_project{QStringLiteral("work"), QColor{1, 2, 3}};
  FullTimePlan m_plan;
  IntervalModel m_interval_model;
  [[nodiscard]] Period period() const
  {
    return Period{m_date, m_date};
  }
};

}  // namespace

TEST(PlanTest, sickness_waives_the_target_up_to_the_hours_missed)
{
  using std::chrono_literals::operator""h;
  using std::chrono_literals::operator""min;
  using enum Plan::Kind;
  const auto whole = Plan::Share::whole();

  // A full sick day waives the whole target, so however little was worked the day comes out
  // neutral -- the target always matches what was booked.
  EXPECT_EQ(0min, PlannedDay(monday, Sick, whole, 0min).planned_time());
  EXPECT_EQ(3h, PlannedDay(monday, Sick, whole, 3h).planned_time());
  EXPECT_EQ(8h, PlannedDay(monday, Sick, whole, 8h).planned_time());
  // But it never waives more than the day was worth, so working through it still earns overtime
  // rather than being swallowed. This is what rules out a flat "a sick day is always eight hours".
  EXPECT_EQ(8h, PlannedDay(monday, Sick, whole, 9h).planned_time());

  // Child sickness behaves identically; only the allowance it draws on differs.
  EXPECT_EQ(0min, PlannedDay(monday, ChildSick, whole, 0min).planned_time());
  EXPECT_EQ(3h, PlannedDay(monday, ChildSick, whole, 3h).planned_time());
  EXPECT_EQ(8h, PlannedDay(monday, ChildSick, whole, 9h).planned_time());
}

TEST(PlanTest, half_sickness_waives_at_most_half_a_day)
{
  using std::chrono_literals::operator""h;
  using std::chrono_literals::operator""min;
  using enum Plan::Kind;

  // Half a day is only half an excuse: not working at all still leaves four hours owed.
  EXPECT_EQ(4h, PlannedDay(monday, Sick, ::half(), 0min).planned_time());
  EXPECT_EQ(4h, PlannedDay(monday, Sick, ::half(), 4h).planned_time());

  // The case this rule was designed around: half a day filed for a sick child, six hours worked
  // anyway because the child slept. Two of the four waivable hours are used and the day is
  // neutral -- filing half a day sets a ceiling on the excuse, not a target to beat.
  EXPECT_EQ(6h, PlannedDay(monday, ChildSick, ::half(), 6h).planned_time());
  EXPECT_EQ(8h, PlannedDay(monday, ChildSick, ::half(), 9h).planned_time());
}

TEST(PlanTest, half_vacation_spends_a_budget_instead_of_waiving_a_target)
{
  using std::chrono_literals::operator""h;
  using std::chrono_literals::operator""min;
  using enum Plan::Kind;

  // The deliberate counterpart to half sickness, and the reason the same share means different
  // things to the two families of Kind: a vacation half-day is spent the moment it is booked, so
  // the target stays at four hours and the two extra hours really are overtime.
  EXPECT_EQ(4h, PlannedDay(monday, Vacation, ::half(), 0min).planned_time());
  EXPECT_EQ(4h, PlannedDay(monday, Vacation, ::half(), 6h).planned_time());
  EXPECT_EQ(0min, PlannedDay(monday, Vacation, Plan::Share::whole(), 6h).planned_time());
}

TEST(PlanTest, sick_counters_report_the_time_actually_waived)
{
  using std::chrono_literals::operator""h;
  using std::chrono_literals::operator""min;
  using enum Plan::Kind;
  const auto whole = Plan::Share::whole();

  EXPECT_EQ(8h, PlannedDay(monday, Sick, whole, 0min).leave_time(Sick));
  // Three hours worked means only the missing five had to be covered.
  EXPECT_EQ(5h, PlannedDay(monday, Sick, whole, 3h).leave_time(Sick));
  // A full day worked despite being sick never draws on the allowance at all.
  EXPECT_EQ(0min, PlannedDay(monday, Sick, whole, 8h).leave_time(Sick));

  // The two allowances are counted separately, which is the point of child sickness being its own
  // kind: it is capped by law, own sickness effectively is not.
  EXPECT_EQ(0min, PlannedDay(monday, ChildSick, whole, 0min).leave_time(Sick));
  EXPECT_EQ(8h, PlannedDay(monday, ChildSick, whole, 0min).leave_time(ChildSick));
  EXPECT_EQ(0min, PlannedDay(monday, Sick, whole, 0min).leave_time(ChildSick));

  // Half a day filed but six hours worked spends two hours of the allowance, not the four filed.
  EXPECT_EQ(2h, PlannedDay(monday, ChildSick, ::half(), 6h).leave_time(ChildSick));
  // The half-day ceiling still holds when nothing was worked.
  EXPECT_EQ(4h, PlannedDay(monday, ChildSick, ::half(), 0min).leave_time(ChildSick));
}

TEST(PlanTest, vacation_counters_report_the_budget_spent)
{
  using std::chrono_literals::operator""h;
  using std::chrono_literals::operator""min;
  using enum Plan::Kind;

  // Unlike the sickness counters these do not depend on what was worked: the budget is gone
  // either way.
  EXPECT_EQ(8h, PlannedDay(monday, Vacation, Plan::Share::whole(), 0min).leave_time(Vacation));
  EXPECT_EQ(8h, PlannedDay(monday, Vacation, Plan::Share::whole(), 6h).leave_time(Vacation));
  EXPECT_EQ(4h, PlannedDay(monday, Holiday, ::half(), 6h).leave_time(Holiday));
  // A day the plan expects no work on is worth nothing, so nothing can be spent on it.
  EXPECT_EQ(0min, PlannedDay(saturday, Vacation, Plan::Share::whole(), 0min).leave_time(Vacation));
}

TEST(PlanTest, sickness_on_a_day_without_planned_work_costs_nothing)
{
  using std::chrono_literals::operator""h;
  using std::chrono_literals::operator""min;
  using enum Plan::Kind;
  const auto whole = Plan::Share::whole();

  // Being sick over a weekend must not eat into a capped allowance: there was no target to waive.
  // This falls out of the clamp rather than needing a special case, but it is worth pinning,
  // because a child-sick allowance silently burnt on a Saturday would be hard to notice.
  EXPECT_EQ(0min, PlannedDay(saturday, ChildSick, whole, 0min).planned_time());
  EXPECT_EQ(0min, PlannedDay(saturday, ChildSick, whole, 0min).leave_time(ChildSick));
  EXPECT_EQ(0min, PlannedDay(saturday, ChildSick, ::half(), 0min).leave_time(ChildSick));
  // Work booked on a sick weekend is overtime, exactly as it would be on a normal weekend.
  EXPECT_EQ(0min, PlannedDay(saturday, Sick, whole, 3h).planned_time());
}

TEST(PlanTest, a_waiver_only_covers_what_a_budget_left_over)
{
  using std::chrono_literals::operator""h;
  using std::chrono_literals::operator""min;
  FullTimePlan plan;
  const auto day = QDate{2026, 9, 22};  // a Tuesday
  ASSERT_TRUE(plan.add(::make_entry(day, day, Plan::Kind::Vacation, ::half())));
  ASSERT_TRUE(plan.add(::make_entry(day, day, Plan::Kind::Sick, ::half())));

  // Falling sick through the half day that was not already vacation: four hours of target remain
  // after the vacation is spent, and the waiver covers exactly those.
  const IntervalModel interval_model;
  const Period period{day, day};
  EXPECT_EQ(0min, plan.planned_working_time(period, interval_model));
  EXPECT_EQ(4h, plan.leave_time(Plan::Kind::Vacation, period, interval_model)) << "the vacation is spent regardless";
  EXPECT_EQ(4h, plan.leave_time(Plan::Kind::Sick, period, interval_model)) << "and the waiver covers the rest";
}
