#include "views/periodsummaryview.h"
#include "tableview.h"
#include "views/periodsummarymodel.h"

PeriodSummaryView::PeriodSummaryView(QWidget* parent)
  : AbstractPeriodView(parent)
  , m_table_view(::setup_ui_with_single_table_view(this))
  , m_model(std::make_unique<PeriodSummaryModel>())

{
  m_table_view.setModel(m_model.get());
  m_table_view.setSelectionMode(QAbstractItemView::NoSelection);
  connect(this, &PeriodSummaryView::interval_model_changed, this, [this]() { m_model->set_source(time_sheet()); });
  connect(this, &PeriodSummaryView::period_changed, this, [this]() { m_model->set_period(current_period()); });
}

PeriodSummaryView::~PeriodSummaryView() = default;

void PeriodSummaryView::invalidate()
{
}
