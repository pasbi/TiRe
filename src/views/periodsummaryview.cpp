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
}

PeriodSummaryView::~PeriodSummaryView() = default;

void PeriodSummaryView::invalidate()
{
}

void PeriodSummaryView::set_model(const TimeSheet* time_sheet)
{
  m_model->set_source(time_sheet);
  AbstractPeriodView::set_model(time_sheet);
}

void PeriodSummaryView::set_period(const Period& period)
{
  m_model->set_period(period);
  AbstractPeriodView::set_period(period);
}
