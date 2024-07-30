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

UndoStack::Macro::Macro(const QString& text, QUndoStack& stack) : m_stack(stack)
{
  m_stack.beginMacro(text);
}

UndoStack::Macro::~Macro()
{
  m_stack.endMacro();
}

std::unique_ptr<UndoStack::Macro> UndoStack::start_macro(const QString& text)
{
  return std::make_unique<Macro>(text, m_impl);
}
