#include "plantableview.h"

#include "application.h"
#include "combobox.h"
#include "commands/commands.h"
#include "commands/undostack.h"
#include "periodedit.h"
#include "plan.h"

#include "enumcombobox.h"
#include "exceptions.h"
#include "views/callbackdelegate.h"

#include <QAction>
#include <QMessageBox>
#include <QSpinBox>
#include <QStyledItemDelegate>
#include <spdlog/spdlog.h>

namespace
{

/** @brief Half of a day, the one share besides a whole one that the editor offers. */
constexpr auto half_day = Plan::Share::from_percent(50);

/** @brief The message shown when the plan refuses a booking. */
[[nodiscard]] QString conflict_message(const Plan::Conflict conflict)
{
  switch (conflict) {
    using enum Plan::Conflict;
  case None:
    return {};
  case Overbooked:
    return QObject::tr("The day would hold more leave than it is long.");
  case MixedSickness:
    return QObject::tr("Your own and your child's sickness cannot share a day, because the hours you missed could "
                       "not be told apart afterwards.");
  case ImpossibleShare:
    return QObject::tr("The share must be between 1% and 100% of a day.");
  }
  Q_UNREACHABLE();
}

class KindDelegate : public QStyledItemDelegate
{
  using Editor = EnumComboBox<Plan::Kind, Plan::kind_count>;
  QWidget* createEditor(QWidget* const parent, const QStyleOptionViewItem&, const QModelIndex& index) const override
  {
    return std::make_unique<Editor>(parent).release();
  }

  void setEditorData(QWidget* const editor, const QModelIndex& index) const override
  {
    dynamic_cast<Editor&>(*editor).set_current_enum(dynamic_cast<const Plan&>(*index.model()).entry(index.row()).kind);
  }

  void setModelData(QWidget* const editor, QAbstractItemModel* const model, const QModelIndex& index) const override
  {
    auto& plan = dynamic_cast<Plan&>(*model);
    const auto& entry = plan.entry(index.row());
    const auto kind = dynamic_cast<const Editor&>(*editor).current_enum();
    if (kind == entry.kind) {
      return;
    }
    // Validate up front: pushing a command whose redo() throws would leave the undo stack holding
    // an entry that never applied.
    if (const auto conflict = plan.check_kind(entry, kind); conflict != Plan::Conflict::None) {
      QMessageBox::critical(editor, tr("Failed to set kind"), ::conflict_message(conflict));
      return;
    }
    Application::undo_stack().push(make_modify_plan_kind_command(plan, entry, kind));
  }
};

class ShareDelegate : public QStyledItemDelegate
{
  QWidget* createEditor(QWidget* const parent, const QStyleOptionViewItem&, const QModelIndex& index) const override
  {
    // The only two shares anybody books. Plan::Share itself takes any fraction, which is what
    // lets a finer split exist later without another migration, but offering one here would only
    // be a way to mistype a day.
    auto editor = std::make_unique<ComboBox>(parent);
    editor->addItem(tr("Full day"), Plan::Share::whole().permille());
    editor->addItem(tr("Half day"), ::half_day.permille());
    return editor.release();
  }

  void setEditorData(QWidget* const editor, const QModelIndex& index) const override
  {
    auto& combo_box = dynamic_cast<QComboBox&>(*editor);
    const auto share = dynamic_cast<const Plan&>(*index.model()).entry(index.row()).share;
    if (combo_box.findData(share.permille()) < 0) {
      // A share this editor could not have produced -- a hand-edited database, say. Offered as it
      // stands rather than silently rounded to whichever preset is nearest.
      combo_box.addItem(QString::fromStdString(fmt::format("{}", share)), share.permille());
    }
    combo_box.setCurrentIndex(combo_box.findData(share.permille()));
  }

  void setModelData(QWidget* const editor, QAbstractItemModel* const model, const QModelIndex& index) const override
  {
    auto& plan = dynamic_cast<Plan&>(*model);
    const auto& entry = plan.entry(index.row());
    const auto share = Plan::Share::from_permille(dynamic_cast<const QComboBox&>(*editor).currentData().toInt());
    if (share == entry.share) {
      return;
    }
    if (const auto conflict = plan.check_share(entry, share); conflict != Plan::Conflict::None) {
      QMessageBox::critical(editor, tr("Failed to set share"), ::conflict_message(conflict));
      return;
    }
    Application::undo_stack().push(make_modify_plan_share_command(plan, entry, share));
  }
};

}  // namespace

PlanTableView::PlanTableView(QWidget* parent)
  : TableView(parent)
  , m_period_delegate(std::make_unique<CallbackDelegate>([this](const QModelIndex& index) { open_period_edit(index); }))
  , m_kind_delegate(std::make_unique<KindDelegate>())
  , m_share_delegate(std::make_unique<ShareDelegate>())
{
  setItemDelegateForColumn(Plan::period_column, m_period_delegate.get());
  setItemDelegateForColumn(Plan::kind_column, m_kind_delegate.get());
  setItemDelegateForColumn(Plan::share_column, m_share_delegate.get());

  // Deletion works on whole entries, so selectedRows() must actually report something.
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSelectionMode(QAbstractItemView::ExtendedSelection);

  setContextMenuPolicy(Qt::ActionsContextMenu);
  auto* const delete_action = new QAction{tr("Delete"), this};
  // Widget-scoped on purpose: PeriodDetailView carries its own window-scoped Delete, and two
  // shortcuts claiming Del in the same window resolve as ambiguous, firing neither.
  delete_action->setShortcut(QKeySequence{Qt::Key_Delete});
  delete_action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  connect(delete_action, &QAction::triggered, this, &PlanTableView::delete_selected_entries);
  addAction(delete_action);
}

void PlanTableView::delete_selected_entries()
{
  // The same reference cast open_period_edit uses: MainWindow sets the model in its constructor,
  // so a PlanTableView the user can reach always has one, and one that is not a Plan is a bug to
  // hear about rather than to return silently from.
  auto& plan = dynamic_cast<Plan&>(*model());
  std::set<const Plan::Entry*> selection;
  for (const auto& index : selectionModel()->selectedRows()) {
    selection.insert(&plan.entry(index.row()));
  }
  delete_plan_entries(plan, selection);
}

void PlanTableView::open_period_edit(const QModelIndex& index)
{
  PeriodEdit period_edit;
  auto& plan = dynamic_cast<Plan&>(*model());
  const auto& entry = plan.entry(index.row());
  period_edit.set_period(entry.period);
  if (period_edit.exec() != QDialog::Accepted) {
    return;
  }
  const auto period = period_edit.period();
  // Validate up front: pushing a command whose redo() throws would leave the undo stack holding
  // an entry that never applied.
  if (const auto conflict = plan.check_period(entry, period); conflict != Plan::Conflict::None) {
    QMessageBox::critical(this, tr("Failed to set period"), ::conflict_message(conflict));
    return;
  }
  Application::undo_stack().push(make_modify_plan_period_command(plan, entry, period));
}
