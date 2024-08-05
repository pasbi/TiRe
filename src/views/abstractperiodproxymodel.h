#pragma once
#include "period.h"

#include <QSortFilterProxyModel>

class IntervalModel;

class AbstractPeriodProxyModel : public QSortFilterProxyModel
{
public:
  void set_source_model(IntervalModel* const model);
  void set_period(const Period& period);

protected:
  [[nodiscard]] const IntervalModel* interval_model() const noexcept;
  [[nodiscard]] const Period& current_period() const noexcept;

private:
  const IntervalModel* m_interval_model = nullptr;
  Period m_period;
};
