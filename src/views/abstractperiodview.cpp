#include "views/abstractperiodview.h"
#include "intervalmodel.h"
#include "views/abstractperiodproxymodel.h"

AbstractPeriodView::AbstractPeriodView(std::unique_ptr<AbstractPeriodProxyModel> proxy_model, QWidget* parent)
  : QWidget(parent), m_proxy_model(std::move(proxy_model))
{
}

AbstractPeriodView::~AbstractPeriodView() = default;

void AbstractPeriodView::set_period_type(const Period::Type type)
{
  m_type = type;
  set_date(QDate::currentDate());
}

void AbstractPeriodView::set_date(const QDate& date)
{
  if (const auto new_period = Period(date, m_type); new_period != m_current_period) {
    m_current_period = new_period;
  } else {
    return;
  }

  m_proxy_model->set_period(m_current_period);
  invalidate();
  Q_EMIT period_changed();
}

void AbstractPeriodView::set_model(IntervalModel& interval_model, const Plan& plan)
{
  m_plan = &plan;
  m_interval_model = &interval_model;
  m_proxy_model->set_source_model(m_interval_model);
  invalidate();
  if (m_interval_model != nullptr) {
    connect(m_interval_model, &IntervalModel::data_changed, this, &AbstractPeriodView::invalidate);
  }
}

const Period& AbstractPeriodView::current_period() noexcept
{
  return m_current_period;
}

const IntervalModel* AbstractPeriodView::interval_model() const noexcept
{
  return m_interval_model;
}

AbstractPeriodProxyModel* AbstractPeriodView::proxy_model() const noexcept
{
  return m_proxy_model.get();
}

const Plan* AbstractPeriodView::plan() const noexcept
{
  return m_plan;
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
