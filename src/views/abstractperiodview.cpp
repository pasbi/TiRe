#include "views/abstractperiodview.h"
#include "intervalmodel.h"
#include "timesheet.h"

AbstractPeriodView::AbstractPeriodView(QWidget* parent) : QWidget(parent)
{
}

AbstractPeriodView::~AbstractPeriodView() = default;

void AbstractPeriodView::set_period_type(const Period::Type type)
{
  m_type = type;
  set_date(QDate::currentDate());
}

void AbstractPeriodView::set_period(const Period& period)
{
  if (period == m_current_period) {
    return;
  }

  m_current_period = period;
  invalidate();
  Q_EMIT period_changed();
}

void AbstractPeriodView::set_date(const QDate& date)
{
  set_period(Period(date, m_type));
}

void AbstractPeriodView::set_model(const TimeSheet* const time_sheet)
{
  m_time_sheet = time_sheet;
  invalidate();
  if (m_time_sheet != nullptr) {
    connect(&m_time_sheet->interval_model(), &IntervalModel::data_changed, this, &AbstractPeriodView::invalidate);
  }
  Q_EMIT interval_model_changed();
}

const Period& AbstractPeriodView::current_period() noexcept
{
  return m_current_period;
}

const TimeSheet* AbstractPeriodView::time_sheet() const
{
  return m_time_sheet;
}
void AbstractPeriodView::next()
{
  set_date(m_current_period.end().addDays(1));
}

void AbstractPeriodView::prev()
{
  set_date(m_current_period.begin().addDays(-1));
}

void AbstractPeriodView::today()
{
  set_date(QDate::currentDate());
}
