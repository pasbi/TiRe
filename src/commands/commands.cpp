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
