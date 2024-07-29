#pragma once

#include <QUndoStack>

class Command;

class UndoStack
{
public:
  [[nodiscard]] const QUndoStack& impl() const noexcept;
  [[nodiscard]] QUndoStack& impl() noexcept;
  void push(std::unique_ptr<Command> command);

private:
  QUndoStack m_impl;
};
