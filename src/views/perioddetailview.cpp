#include "views/perioddetailview.h"
#include "intervalmodel.h"
#include "tableview.h"
#include "timesheet.h"
#include "views/perioddetailproxymodel.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPainter>
#include <QStyledItemDelegate>
#include <spdlog/spdlog.h>

namespace
{

class ItemDelegate : public QStyledItemDelegate
{
public:
  void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
  {
    auto option_copy = option;
    if (option.state & QStyle::State_Selected) {
      option_copy.font.setBold(true);
      option_copy.font.setUnderline(true);
      option_copy.palette.setBrush(QPalette::Highlight, index.data(Qt::BackgroundRole).value<QBrush>());
      option_copy.palette.setBrush(QPalette::HighlightedText, index.data(Qt::ForegroundRole).value<QBrush>());
    }
    QStyledItemDelegate::paint(painter, option_copy, index);
  }
};

}  // namespace

PeriodDetailView::PeriodDetailView(QWidget* parent)
  : AbstractPeriodView(parent)
  , m_table_view(::setup_ui_with_single_table_view(this))
  , m_proxy_model(std::make_unique<PeriodDetailProxyModel>())
  , m_item_delegate(std::make_unique<ItemDelegate>())
{
  connect(&m_table_view, &QAbstractItemView::doubleClicked, this,
          [this](const QModelIndex& index) { Q_EMIT double_clicked(m_proxy_model->mapToSource(index)); });
  m_table_view.setModel(m_proxy_model.get());
  m_table_view.setSelectionBehavior(QAbstractItemView::SelectRows);
  m_table_view.setSelectionMode(QAbstractItemView::SingleSelection);
  m_table_view.setItemDelegate(m_item_delegate.get());
}

PeriodDetailView::~PeriodDetailView() = default;

const Interval* PeriodDetailView::current_interval() const
{
  if (const auto index = m_table_view.currentIndex(); index.isValid()) {
    const auto row = m_proxy_model->mapToSource(index).row();
    return time_sheet()->interval_model().interval(row);
  }
  return nullptr;
}

std::set<const Interval*> PeriodDetailView::selected_intervals() const
{
  std::set<const Interval*> selection;
  for (const auto& index : m_table_view.selectionModel()->selectedRows()) {
    const auto row = m_proxy_model->mapToSource(index).row();
    selection.insert(time_sheet()->interval_model().interval(row));
  }
  return selection;
}

void PeriodDetailView::set_model(const TimeSheet* time_sheet)
{
  m_proxy_model->set_source_model(time_sheet == nullptr ? nullptr : &time_sheet->interval_model());
  AbstractPeriodView::set_model(time_sheet);
}

void PeriodDetailView::set_period(const Period& period)
{
  m_proxy_model->set_period(period);
  AbstractPeriodView::set_period(period);
}

void PeriodDetailView::invalidate()
{
  update();
}
