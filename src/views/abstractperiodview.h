#pragma once

#include "period.h"
#include <QWidget>

class AbstractPeriodProxyModel;
class IntervalModel;
class Plan;
class TimeSheet;

class AbstractPeriodView : public QWidget
{
  Q_OBJECT
protected:
  explicit AbstractPeriodView(std::unique_ptr<AbstractPeriodProxyModel> proxy_model, QWidget* parent = nullptr);
  ~AbstractPeriodView() override;
  [[nodiscard]] const IntervalModel* interval_model() const noexcept;
  [[nodiscard]] AbstractPeriodProxyModel* proxy_model() const noexcept;

public:
  void set_period_type(Period::Type type);
  void set_period(const Period& period);
  void set_date(const QDate& date);
  void set_model(const TimeSheet& time_sheet);
  [[nodiscard]] const Period& current_period() noexcept;
  virtual void invalidate() = 0;
  [[nodiscard]] const Plan* plan() const noexcept;
  void set_plan(const Plan* plan) noexcept;

  void next();
  void prev();
  void today();

Q_SIGNALS:
  void period_changed();

private:
  Period::Type m_type = Period::Type::Day;
  Period m_current_period;
  IntervalModel* m_interval_model = nullptr;
  const Plan* m_plan;
  std::unique_ptr<AbstractPeriodProxyModel> m_proxy_model;
};
