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

private:
  std::unique_ptr<Ui::PlanView> m_ui;
};
