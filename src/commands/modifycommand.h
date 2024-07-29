#pragma once

#include "commands/command.h"
#include "interval.h"
#include "intervalmodel.h"

class IntervalModel;

class ModifyBase
{
public:
  virtual ~ModifyBase() = default;
  virtual void swap() = 0;
};

template<typename Object, typename Value, typename Swapper, typename Signal> class ModifyCommandHelper final
  : public ModifyBase
{
public:
  explicit ModifyCommandHelper(Object& object, Value other_value, Swapper setter, Signal signal)
    : m_object(object), m_other_value(std::move(other_value)), m_swapper(std::move(setter)), m_signal(signal)
  {
  }

  void swap() override
  {
    static_assert(!std::is_reference_v<decltype(std::invoke(m_swapper, m_object, m_other_value))>);
    m_other_value = std::invoke(m_swapper, m_object, m_other_value);
    std::invoke(m_signal);
  }

private:
  Object& m_object;
  Value m_other_value;
  const Swapper m_swapper;
  const Signal m_signal;
};

class Interval;

// TODO generalise
template<typename Value, typename Swapper> class ModifyIntervalCommand final : public Command
{
public:
  explicit ModifyIntervalCommand(IntervalModel& interval_model, Interval& interval, Value other_value, Swapper swapper)
    : Command()
    , m_modify_base(
          new ModifyCommandHelper(interval, std::move(other_value), std::move(swapper), [&interval_model, &interval]() {
            const auto index = interval_model.index(interval);
            Q_EMIT interval_model.dataChanged(index, index);
            Q_EMIT interval_model.data_changed();
          }))
  {
  }

  void undo() override
  {
    m_modify_base->swap();
  }

  void redo() override
  {
    m_modify_base->swap();
  }

  std::unique_ptr<ModifyBase> m_modify_base;
};
