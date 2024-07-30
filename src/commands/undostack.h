#pragma once

#include <QUndoStack>

class Command;

class UndoStack
{
public:
  [[nodiscard]] const QUndoStack& impl() const noexcept;
  [[nodiscard]] QUndoStack& impl() noexcept;
  void push(std::unique_ptr<Command> command);

  class Macro
  {
  public:
    explicit Macro(const QString& text, QUndoStack& stack);
    ~Macro();

  private:
    QUndoStack& m_stack;
  };

  std::unique_ptr<Macro> start_macro(const QString& text);

private:
  QUndoStack m_impl;
};
