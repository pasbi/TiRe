#include "commands/undostack.h"
#include "commands/command.h"

const QUndoStack& UndoStack::impl() const noexcept
{
  return m_impl;
}

QUndoStack& UndoStack::impl() noexcept
{
  return m_impl;
}

void UndoStack::push(std::unique_ptr<Command> command)
{
  m_impl.push(command.release());
}
