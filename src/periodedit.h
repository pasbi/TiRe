#pragma once

#include "period.h"

#include <QDialog>

namespace Ui
{
class PeriodEdit;
}

class PeriodEdit : public QDialog
{
  Q_OBJECT

public:
  explicit PeriodEdit(QWidget* parent = nullptr);
  ~PeriodEdit() override;

  void set_period(const Period& period) const;
  void set_type(Period::Type type) const;
  [[nodiscard]] Period::Type type() const noexcept;
  [[nodiscard]] Period period() const noexcept;

private:
  std::unique_ptr<Ui::PeriodEdit> m_ui;
};
