#pragma once

#include "period.h"
#include <QWidget>

class AbstractPeriodProxyModel;
class Plan;
class IntervalModel;

class AbstractPeriodView : public QWidget
{
  Q_OBJECT
protected:
  explicit AbstractPeriodView(std::unique_ptr<AbstractPeriodProxyModel> proxy_model, QWidget* parent = nullptr);
  ~AbstractPeriodView() override;
  [[nodiscard]] const IntervalModel* interval_model() const noexcept;
  [[nodiscard]] AbstractPeriodProxyModel* proxy_model() const noexcept;
  [[nodiscard]] const Plan* plan() const noexcept;

public:
  void set_period_type(Period::Type type);
  void set_date(const QDate& date);
  void set_model(IntervalModel& interval_model, const Plan& plan);
  [[nodiscard]] const Period& current_period() noexcept;
  virtual void invalidate() = 0;

  void next();
  void prev();
  void today();

Q_SIGNALS:
  void period_changed();

private:
  Period::Type m_type = Period::Type::Day;
  Period m_current_period;
  IntervalModel* m_interval_model = nullptr;
  std::unique_ptr<AbstractPeriodProxyModel> m_proxy_model;
  const Plan* m_plan = nullptr;  // TODO plan could be in a child class
};
