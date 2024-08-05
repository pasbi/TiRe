#pragma once

#include "period.h"
#include "views/abstractperiodview.h"
#include <set>

class IntervalModel;
class Plan;

namespace Ui
{
class PeriodDetailView;
}

class PeriodDetailView final : public AbstractPeriodView
{
  Q_OBJECT

public:
  explicit PeriodDetailView(QWidget* parent = nullptr);
  ~PeriodDetailView() override;
  void invalidate() override;
  [[nodiscard]] const Interval* current_interval() const;
  [[nodiscard]] std::set<const Interval*> selected_intervals() const;

Q_SIGNALS:
  void double_clicked(const QModelIndex& index);

private:
  std::unique_ptr<Ui::PeriodDetailView> m_ui;
  void clear() const;
};
