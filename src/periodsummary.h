#pragma once

#include "period.h"
#include <QWidget>

class IntervalModel;

namespace Ui
{
class PeriodSummary;
}

class PeriodSummary : public QWidget
{
  Q_OBJECT

public:
  explicit PeriodSummary(QWidget* parent = nullptr);
  ~PeriodSummary() override;

  void set_period_type(Period::Type type);
  void set_date(const QDate& date);
  void set_model(IntervalModel& interval_model);

private:
  void recalculate();
  std::unique_ptr<Ui::PeriodSummary> m_ui;
  Period::Type m_type = Period::Type::Day;
  Period m_current_period;
  IntervalModel* m_interval_model = nullptr;
  void clear() const;
  class ProxyModel;
  std::unique_ptr<ProxyModel> m_proxy_model;
};
