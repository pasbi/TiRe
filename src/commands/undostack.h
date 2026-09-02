#pragma once

#include "db/database.h"

#include <QObject>
#include <QUndoStack>
#include <functional>

class Command;

/**
 * @class UndoStack undostack.h "commands/undostack.h"
 * @brief The application's undo stack, and the transaction boundary around every edit.
 *
 * Each top-level entry -- a push, an undo, a redo, or a whole macro -- runs inside one database
 * transaction, so a compound action such as deleting forty intervals either lands completely or
 * not at all, and costs one commit rather than forty.
 */
class UndoStack : public QObject
{
  Q_OBJECT
public:
  [[nodiscard]] const QUndoStack& impl() const noexcept;
  [[nodiscard]] QUndoStack& impl() noexcept;

  void push(std::unique_ptr<Command> command);

  /**
   * @name Undoing and redoing
   * These exist so that undo and redo get a transaction too. QUndoStack::undo()/redo() would
   * bypass push() entirely, and undoing a macro would then run one transaction per child command.
   * @{
   */
  void undo();
  void redo();
  /** @} */

  class Macro
  {
  public:
    explicit Macro(const QString& text, UndoStack& stack);
    ~Macro();
    Macro(const Macro&) = delete;
    Macro& operator=(const Macro&) = delete;
    Macro(Macro&&) = delete;
    Macro& operator=(Macro&&) = delete;

  private:
    UndoStack& m_stack;
    std::unique_ptr<Database::Transaction> m_transaction;
  };

  [[nodiscard]] std::unique_ptr<Macro> start_macro(const QString& text);

Q_SIGNALS:
  /** @brief An edit could not be written. The in-memory state is ahead of the database. */
  void write_failed(const QString& message);
  /** @brief An edit was written successfully; any previous failure is over. */
  void write_succeeded();

private:
  /** @brief Runs @p action inside a transaction, reporting rather than propagating failures. */
  void run_in_transaction(const std::function<void()>& action, const QString& what);

  QUndoStack m_impl;
};
