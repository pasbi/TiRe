#pragma once

#include "commands/modifycommand.h"
#include "intervalmodel.h"
#include "plan.h"

#include <memory>
#include <set>

class Command;
class Interval;
class IntervalModel;

void delete_intervals(IntervalModel& interval_model, const std::set<const Interval*>& selection);
void delete_plan_entries(Plan& plan, const std::set<const Plan::Entry*>& selection);
void split_interval(IntervalModel& interval_model, const Interval& interval);

/**
 * @name Plan edit commands
 * Plan::swap_kind and Plan::swap_period already persist and emit, so these carry no extra signal.
 * @{
 */
[[nodiscard]] std::unique_ptr<Command> make_modify_plan_kind_command(Plan& plan, const Plan::Entry& entry,
                                                                     Plan::Kind kind);
/** @pre Plan::can_set_period(entry, period) -- redo() must not throw. */
[[nodiscard]] std::unique_ptr<Command> make_modify_plan_period_command(Plan& plan, const Plan::Entry& entry,
                                                                       Period period);
[[nodiscard]] std::unique_ptr<Command> make_modify_plan_start_command(Plan& plan, QDate start);
[[nodiscard]] std::unique_ptr<Command> make_modify_plan_overtime_offset_command(Plan& plan,
                                                                                std::chrono::minutes offset);
/** @} */

template<typename IntervalT, typename Value, typename Swapper> std::unique_ptr<Command>
make_modify_interval_command(IntervalModel& interval_model, IntervalT& interval, Value other_value, Swapper swapper)
{
  if constexpr (std::is_const_v<IntervalT>) {
    return make_modify_interval_command(interval_model, interval_model.remove_const(interval), std::move(other_value),
                                        std::move(swapper));
  } else {
    const auto signal = [&interval_model, &interval]() {
      // Persist before notifying, so no observer can see state the database does not have. This
      // is the write-through hook for Interval's swap_* setters, which are themselves unaware of
      // persistence; it covers undo as well, because ModifyCommand::undo() calls redo().
      interval_model.persist(interval);
      const auto index = interval_model.index(interval);
      Q_EMIT interval_model.dataChanged(index, index.siblingAtColumn(interval_model.columnCount({}) - 1));
      Q_EMIT interval_model.data_changed();
    };
    return make_modify_command(interval, std::move(other_value), std::move(swapper), std::move(signal));
  }
}
