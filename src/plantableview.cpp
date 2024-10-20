#include "plantableview.h"

#include "periodedit.h"
#include "plan.h"

#include "enumcombobox.h"
#include "views/callbackdelegate.h"

#include <QComboBox>
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
    dynamic_cast<Plan&>(*model).set_data(index.row(), dynamic_cast<const Editor&>(*editor).current_enum());
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
}

void PlanTableView::open_period_edit(const QModelIndex& index) const
{
  PeriodEdit period_edit;
  auto& plan = dynamic_cast<Plan&>(*model());
  period_edit.set_period(plan.entry(index.row()).period);
  if (period_edit.exec() == QDialog::Accepted) {
    plan.set_data(index.row(), period_edit.period());
  }
}
