#pragma once

#include "period.h"
#include <QSortFilterProxyModel>

class IntervalModel;

class PeriodDetailProxyModel : public QSortFilterProxyModel
{
public:
  explicit PeriodDetailProxyModel(QObject *parent = nullptr);
  void set_source_model(IntervalModel* const model);
  void set_period(const Period& period);

protected:
  [[nodiscard]] const IntervalModel* interval_model() const noexcept;
  [[nodiscard]] const Period& current_period() const noexcept;
  [[nodiscard]] bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

private:
  const IntervalModel* m_interval_model = nullptr;
  Period m_period;
};
