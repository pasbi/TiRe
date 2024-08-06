#include "views/perioddetailview.h"
#include "intervalmodel.h"
#include "plan.h"
#include "tableview.h"
#include "timesheet.h"
#include "views/perioddetailproxymodel.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <spdlog/spdlog.h>

PeriodDetailView::PeriodDetailView(QWidget* parent)
  : AbstractPeriodView(parent)
  , m_table_view(::setup_ui_with_single_table_view(this))
  , m_proxy_model(std::make_unique<PeriodDetailProxyModel>())

{
  connect(&m_table_view, &QAbstractItemView::doubleClicked, this,
          [this](const QModelIndex& index) { Q_EMIT double_clicked(m_proxy_model->mapToSource(index)); });
  connect(this, &PeriodDetailView::period_changed, this, [this]() { m_proxy_model->set_period(current_period()); });
  connect(this, &PeriodDetailView::period_changed, this, [this]() {
    m_proxy_model->set_source_model(time_sheet() == nullptr ? nullptr : &time_sheet()->interval_model());
  });
  m_table_view.setModel(m_proxy_model.get());
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

void PeriodDetailView::invalidate()
{
  update();
}
