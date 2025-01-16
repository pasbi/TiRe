#pragma once

#include <QSpinBox>

class TimeEditSpinBox : public QSpinBox
{
  Q_OBJECT
public:
  using QSpinBox::QSpinBox;
  void stepBy(int steps) override;
  [[nodiscard]] QString textFromValue(int value) const override;
Q_SIGNALS:
  void wrapped(int direction);
};
