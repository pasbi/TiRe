#include "plantableview.h"

#include "application.h"
#include "commands/commands.h"
#include "commands/undostack.h"
#include "periodedit.h"
#include "plan.h"

#include "enumcombobox.h"
#include "exceptions.h"
#include "views/callbackdelegate.h"

#include <QAction>
#include <QComboBox>
#include <QMessageBox>
#include <QStyledItemDelegate>
#include <spdlog/spdlog.h>

namespace
{

class KindDelegate : public QStyledItemDelegate
{
  using Editor = EnumComboBox<Plan::Kind, 7>;
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
    Application::undo_stack().push(make_modify_plan_kind_command(plan, entry, kind));
  }
};

}  // namespace

PlanTableView::PlanTableView(QWidget* parent)
  : TableView(parent)
  , m_period_delegate(std::make_unique<CallbackDelegate>([this](const QModelIndex& index) { open_period_edit(index); }))
  , m_kind_delegate(std::make_unique<KindDelegate>())
{
  setItemDelegateForColumn(0, m_period_delegate.get());
  setItemDelegateForColumn(1, m_kind_delegate.get());

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
  auto* const plan = dynamic_cast<Plan*>(model());
  if (plan == nullptr) {
    return;
  }
  std::set<const Plan::Entry*> selection;
  for (const auto& index : selectionModel()->selectedRows()) {
    selection.insert(&plan->entry(index.row()));
  }
  delete_plan_entries(*plan, selection);
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
  if (!plan.can_set_period(entry, period)) {
    QMessageBox::critical(this, tr("Failed to set period"), tr("The period would overlap another one."));
    return;
  }
  Application::undo_stack().push(make_modify_plan_period_command(plan, entry, period));
}
