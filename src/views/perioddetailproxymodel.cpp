#include "views/perioddetailproxymodel.h"
#include "intervalmodel.h"

void PeriodDetailProxyModel::set_source_model(IntervalModel* const model)
{
  m_interval_model = model;
  setSourceModel(model);
}

void PeriodDetailProxyModel::set_period(const Period& period)
{
  m_period = period;
  invalidate();
}

const IntervalModel* PeriodDetailProxyModel::interval_model() const noexcept
{
  return m_interval_model;
}

const Period& PeriodDetailProxyModel::current_period() const noexcept
{
  return m_period;
}

bool PeriodDetailProxyModel::filterAcceptsRow(const int source_row, const QModelIndex& source_parent) const
{
  if (interval_model() == nullptr) {
    return false;
  }
  const auto* const interval = interval_model()->intervals().at(source_row);
  const Period period(interval->begin().date(),
                      (interval->end().isValid() ? interval->end() : interval->begin()).date());
  return current_period().contains(period);
}
