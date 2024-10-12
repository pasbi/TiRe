#include "plantableview.h"

#include "periodedit.h"
#include "plan.h"

#include <QComboBox>
#include <QStyledItemDelegate>
#include <spdlog/spdlog.h>

namespace
{

class PeriodDelegate : public QStyledItemDelegate
{
public:
  PeriodDelegate(std::function<void(const QModelIndex&)> edit_callback, QObject* parent = nullptr)
    : QStyledItemDelegate(parent), m_edit_callback(std::move(edit_callback))
  {
  }

  QWidget* createEditor(QWidget* const, const QStyleOptionViewItem&, const QModelIndex& index) const override
  {
    if (m_edit_callback) {
      m_edit_callback(index);
    }
    return nullptr;
  }

private:
  std::function<void(const QModelIndex&)> m_edit_callback;
};

class KindDelegate : public QStyledItemDelegate
{
  QWidget* createEditor(QWidget* const parent, const QStyleOptionViewItem&, const QModelIndex& index) const override
  {
    auto cb = std::make_unique<QComboBox>(parent);
    for (int i = 0; i < 7; ++i) {
      cb->addItem(QString::fromStdString(fmt::format("{}", static_cast<Plan::Kind>(i))));
    }
    return cb.release();
  }

  void setEditorData(QWidget* const editor, const QModelIndex& index) const override
  {
    const auto kind = static_cast<const Plan&>(*index.model()).entry(index.row()).kind;
    static_cast<QComboBox*>(editor)->setCurrentIndex(static_cast<int>(kind));
  }

  void setModelData(QWidget* const editor, QAbstractItemModel* const model, const QModelIndex& index) const override
  {
    const auto kind = static_cast<Plan::Kind>(static_cast<const QComboBox&>(*editor).currentIndex());
    static_cast<Plan&>(*model).set_data(index.row(), kind);
  }
};

}  // namespace

PlanTableView::PlanTableView(QWidget* parent)
  : TableView(parent)
  , m_period_delegate(std::make_unique<PeriodDelegate>([this](const QModelIndex& index) { open_period_edit(index); }))
  , m_kind_delegate(std::make_unique<KindDelegate>())
{
  setItemDelegateForColumn(0, m_period_delegate.get());
  setItemDelegateForColumn(1, m_kind_delegate.get());
}

void PlanTableView::open_period_edit(const QModelIndex& index)
{
  PeriodEdit period_edit;
  auto& plan = static_cast<Plan&>(*model());
  period_edit.set_period(plan.entry(index.row()).period);
  if (period_edit.exec() == QDialog::Accepted) {
    plan.set_data(index.row(), period_edit.period());
  }
}
