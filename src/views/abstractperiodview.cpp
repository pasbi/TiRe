#include "views/abstractperiodview.h"
#include "intervalmodel.h"
#include "timesheet.h"

AbstractPeriodView::AbstractPeriodView(QWidget* parent) : QWidget(parent)
{
}

AbstractPeriodView::~AbstractPeriodView() = default;

void AbstractPeriodView::set_period(const Period& period)
{
  if (period == m_current_period) {
    return;
  }

  m_current_period = period;
  invalidate();
}

void AbstractPeriodView::set_model(const TimeSheet* const time_sheet)
{
  m_time_sheet = time_sheet;
  invalidate();
  if (m_time_sheet != nullptr) {
    connect(&m_time_sheet->interval_model(), &IntervalModel::data_changed, this, &AbstractPeriodView::invalidate);
  }
}

const Period& AbstractPeriodView::current_period() const noexcept
{
  return m_current_period;
}

const TimeSheet* AbstractPeriodView::time_sheet() const
{
  return m_time_sheet;
}
