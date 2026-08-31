#include "commands/commands.h"

#include "addremovecommand.h"
#include "application.h"
#include "interval.h"
#include "splitpointeditor.h"
#include "undostack.h"

void delete_intervals(IntervalModel& interval_model, const std::set<const Interval*>& selection)
{
  const auto macro = Application::undo_stack().start_macro(QObject::tr("Delete selected intervals"));
  for (const auto* const interval : selection) {
    Application::undo_stack().push(make<RemoveCommand>(interval_model, *interval));
  }
}

void delete_plan_entries(Plan& plan, const std::set<const Plan::Entry*>& selection)
{
  if (selection.empty()) {
    // An empty macro still lands on the undo stack as a no-op step and opens a transaction for
    // nothing, so pressing Delete with no selection must not get that far.
    return;
  }
  const auto macro = Application::undo_stack().start_macro(QObject::tr("Delete selected plan entries"));
  for (const auto* const entry : selection) {
    Application::undo_stack().push(make<RemoveCommand>(plan, *entry));
  }
}

// Both capture the entry, not its row: changing a period re-sorts the plan, so by the time undo
// runs, the original row index would point at a different entry.
std::unique_ptr<Command> make_modify_plan_kind_command(Plan& plan, const Plan::Entry& entry, const Plan::Kind kind)
{
  const auto swapper = [&entry](Plan& p, const Plan::Kind k) { return p.swap_kind(entry, k); };
  return make_modify_command(plan, kind, swapper, [] {});
}

std::unique_ptr<Command> make_modify_plan_period_command(Plan& plan, const Plan::Entry& entry, Period period)
{
  const auto swapper = [&entry](Plan& p, Period q) { return p.swap_period(entry, q); };
  return make_modify_command(plan, period, swapper, [] {});
}

std::unique_ptr<Command> make_modify_plan_start_command(Plan& plan, QDate start)
{
  const auto swapper = [](Plan& p, QDate s) { return p.swap_start(s); };
  return make_modify_command(plan, start, swapper, [] {});
}

std::unique_ptr<Command> make_modify_plan_overtime_offset_command(Plan& plan, const std::chrono::minutes offset)
{
  const auto swapper = [](Plan& p, const std::chrono::minutes o) { return p.swap_overtime_offset(o); };
  return make_modify_command(plan, offset, swapper, [] {});
}

void split_interval(IntervalModel& interval_model, const Interval& interval)
{
  SplitPointEditor e;
  e.set_range(interval.begin(), interval.end());

  if (e.exec() == QDialog::Accepted) {
    const auto macro = Application::undo_stack().start_macro(QObject::tr("Delete selected intervals"));
    auto new_interval = std::make_unique<Interval>(interval.project());
    new_interval->swap_begin(e.split_point());
    new_interval->swap_end(interval.end());
    Application::undo_stack().push(make<AddCommand>(interval_model, std::move(new_interval)));
    Application::undo_stack().push(
        make_modify_interval_command(interval_model, interval, e.split_point(), &Interval::swap_end));
  }
}
