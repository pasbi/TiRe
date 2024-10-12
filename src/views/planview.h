#pragma once

#include "abstractperiodview.h"
#include <memory>

namespace Ui
{
class PlanView;
}  // namespace Ui

class PlanView : public AbstractPeriodView
{
public:
  explicit PlanView(QWidget* parent = nullptr);
  ~PlanView() override;
  void clear() const;
  void invalidate() override;
  [[nodiscard]] QSize sizeHint() const override;

private:
  std::unique_ptr<Ui::PlanView> m_ui;
  static int m_max_period_text_width;
  [[nodiscard]] QString period_text(const Period& period) const;
};
