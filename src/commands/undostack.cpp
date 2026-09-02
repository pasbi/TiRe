#include "commands/undostack.h"
#include "application.h"
#include "commands/command.h"
#include "exceptions.h"

#include <spdlog/spdlog.h>

const QUndoStack& UndoStack::impl() const noexcept
{
  return m_impl;
}

QUndoStack& UndoStack::impl() noexcept
{
  return m_impl;
}

void UndoStack::run_in_transaction(const std::function<void()>& action, const QString& what)
{
  auto* const database = Application::database();
  if (database == nullptr) {
    // No database configured, as in tests and benchmarks: just perform the change.
    action();
    return;
  }

  try {
    Database::Transaction transaction{*database};
    action();
    transaction.commit();
    // Only the outermost scope actually commits. Reporting success from an inner one -- inside a
    // macro -- would clear a failure indicator before anything had reached the disk.
    if (!database->in_transaction()) {
      Q_EMIT write_succeeded();
    }
  } catch (const DatabaseError& e) {
    // The transaction has rolled back, so the database is consistent -- but the in-memory change
    // has already happened and is deliberately left in place. A user who can still see the edit
    // and is told loudly that it was not saved can copy it out; one whose edit silently vanishes
    // from the screen cannot.
    //
    // Note this must not try to "undo" the failed command to compensate: QUndoStack::push()
    // executes redo() *before* appending, so a command whose redo() threw is not on the stack,
    // and undoing here would revert the previous, perfectly good command instead.
    spdlog::error("{} could not be written: {}", what.toStdString(), e.what());
    Q_EMIT write_failed(QString::fromStdString(e.what()));
  }
}

void UndoStack::push(std::unique_ptr<Command> command)
{
  auto* const raw_command = command.release();
  run_in_transaction(
      [this, raw_command] {
        try {
          m_impl.push(raw_command);
        } catch (...) {
          // push() executes redo() first and only then takes ownership, so a throwing redo()
          // leaves the command with us.
          delete raw_command;
          throw;
        }
      },
      QObject::tr("The change"));
}

void UndoStack::undo()
{
  run_in_transaction([this] { m_impl.undo(); }, QObject::tr("Undoing the change"));
}

void UndoStack::redo()
{
  run_in_transaction([this] { m_impl.redo(); }, QObject::tr("Redoing the change"));
}

UndoStack::Macro::Macro(const QString& text, UndoStack& stack) : m_stack(stack)
{
  // One transaction spanning the whole macro. The per-push transactions inside collapse into it
  // by depth counting, so the compound action commits exactly once.
  if (auto* const database = Application::database(); database != nullptr) {
    try {
      m_transaction = std::make_unique<Database::Transaction>(*database);
    } catch (const DatabaseError& e) {
      // Every caller is a Qt slot or a dialog's accept(), so letting this escape would unwind
      // through the event loop and terminate. Report it like any other write failure instead; the
      // individual pushes inside will fail and report on their own too.
      spdlog::error("Cannot begin a transaction for '{}': {}", text.toStdString(), e.what());
      Q_EMIT m_stack.write_failed(QString::fromStdString(e.what()));
    }
  }
  m_stack.impl().beginMacro(text);
}

UndoStack::Macro::~Macro()
{
  // endMacro first: beginMacro/endMacro only group commands that already ran, so nothing executes
  // after the commit.
  m_stack.impl().endMacro();
  if (m_transaction == nullptr) {
    return;
  }

  auto failure = QString{};
  try {
    m_transaction->commit();
  } catch (const DatabaseError& e) {
    spdlog::error("The compound change could not be written: {}", e.what());
    failure = QString::fromStdString(e.what());
  }
  // Destroy the transaction before notifying: the failure handler opens a modal dialog, which
  // spins a nested event loop, and that must not happen while an open transaction is still alive.
  m_transaction.reset();
  failure.isEmpty() ? Q_EMIT m_stack.write_succeeded() : Q_EMIT m_stack.write_failed(failure);
}

std::unique_ptr<UndoStack::Macro> UndoStack::start_macro(const QString& text)
{
  return std::make_unique<Macro>(text, *this);
}
