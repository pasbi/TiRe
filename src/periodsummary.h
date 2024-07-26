#pragma once

#include "period.h"
#include <QWidget>

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

private:
  std::unique_ptr<Ui::PeriodSummary> m_ui;
  Period::Type m_type;
};
