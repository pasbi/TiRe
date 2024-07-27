#pragma once

#include "period.h"
#include <QWidget>

class Model;

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
  void set_model(Model& model);

private:
  void recalculate();
  std::unique_ptr<Ui::PeriodSummary> m_ui;
  Period::Type m_type = Period::Type::Day;
  Period m_current_period;
  Model* m_model = nullptr;
  void clear();
  class ProxyModel;
  std::unique_ptr<ProxyModel> m_proxy_model;
};
