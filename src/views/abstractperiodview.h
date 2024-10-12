#pragma once

#include "period.h"
#include <QWidget>

class IntervalModel;
class Plan;
class TimeSheet;

class AbstractPeriodView : public QWidget
{
  Q_OBJECT
protected:
  explicit AbstractPeriodView(QWidget* parent = nullptr);
  ~AbstractPeriodView() override;

public:
  virtual void set_period(const Period& period);
  virtual void set_model(const TimeSheet* time_sheet);
  [[nodiscard]] const Period& current_period() const noexcept;
  virtual void invalidate() = 0;
  [[nodiscard]] const TimeSheet* time_sheet() const;

private:
  Period m_current_period;
  const TimeSheet* m_time_sheet = nullptr;
};
