#pragma once

#include "period.h"
#include "views/abstractperiodview.h"
#include <set>

class IntervalModel;
class Plan;

namespace Ui
{
class PeriodSummary;
}

class PeriodSummary final : public AbstractPeriodView  // TODO this is actually the PeriodDetailView
{
  Q_OBJECT

public:
  explicit PeriodSummary(QWidget* parent = nullptr);
  ~PeriodSummary() override;
  void invalidate() override;
  [[nodiscard]] const Interval* current_interval() const;
  [[nodiscard]] std::set<const Interval*> selected_intervals() const;

Q_SIGNALS:
  void double_clicked(const QModelIndex& index);

private:
  std::unique_ptr<Ui::PeriodSummary> m_ui;
  void clear() const;
};
