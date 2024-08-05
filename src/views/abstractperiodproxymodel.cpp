#include "views/abstractperiodproxymodel.h"
#include "intervalmodel.h"

void AbstractPeriodProxyModel::set_source_model(IntervalModel* const model)
{
  m_interval_model = model;
  setSourceModel(model);
}

void AbstractPeriodProxyModel::set_period(const Period& period)
{
  m_period = period;
  invalidate();
}

const IntervalModel* AbstractPeriodProxyModel::interval_model() const noexcept
{
  return m_interval_model;
}

const Period& AbstractPeriodProxyModel::current_period() const noexcept
{
  return m_period;
}
