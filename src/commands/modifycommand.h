#pragma once

#include "commands/command.h"

template<typename Object, typename Value, typename Swapper, typename Signal> class ModifyCommand final : public Command
{
public:
  explicit ModifyCommand(Object& object, Value other_value, Swapper setter, Signal signal)
    : m_object(object), m_other_value(std::move(other_value)), m_swapper(std::move(setter)), m_signal(signal)
  {
  }

  void redo() override
  {
    static_assert(!std::is_reference_v<decltype(std::invoke(m_swapper, m_object, m_other_value))>);
    m_other_value = std::invoke(m_swapper, m_object, m_other_value);
    std::invoke(m_signal);
  }

  void undo() override
  {
    redo();
  }

private:
  Object& m_object;
  Value m_other_value;
  const Swapper m_swapper;
  const Signal m_signal;
};

template<typename Object, typename Value, typename Swapper, typename Signal>
auto make_modify_command(Object& object, Value other_value, Swapper swapper, Signal signal)
{
  return std::unique_ptr<Command>(
      new ModifyCommand(object, std::move(other_value), std::move(swapper), std::move(signal)));
}
