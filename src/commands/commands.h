#pragma once

#include "commands/modifycommand.h"
#include "intervalmodel.h"

#include <memory>
#include <set>

class Command;
class Interval;
class IntervalModel;

void delete_intervals(IntervalModel& interval_model, const std::set<const Interval*>& selection);
void split_interval(IntervalModel& interval_model, const Interval& interval);

template<typename IntervalT, typename Value, typename Swapper> std::unique_ptr<Command>
make_modify_interval_command(IntervalModel& interval_model, IntervalT& interval, Value other_value, Swapper swapper)
{
  if constexpr (std::is_const_v<IntervalT>) {
    return make_modify_interval_command(interval_model, interval_model.remove_const(interval), std::move(other_value),
                                        std::move(swapper));
  } else {
    const auto signal = [&interval_model, &interval]() {
      const auto index = interval_model.index(interval);
      Q_EMIT interval_model.dataChanged(index, index.siblingAtColumn(interval_model.columnCount({}) - 1));
      Q_EMIT interval_model.data_changed();
    };
    return make_modify_command(interval, std::move(other_value), std::move(swapper), std::move(signal));
  }
}
