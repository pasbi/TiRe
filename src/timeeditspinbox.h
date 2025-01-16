#pragma once

#include <QSpinBox>

class TimeEditSpinBox : public QSpinBox
{
  Q_OBJECT
public:
  using QSpinBox::QSpinBox;
  void stepBy(int steps) override;
  [[nodiscard]] QString textFromValue(int value) const override;

  void set_block_wrap_up(bool blocked);
  void set_block_wrap_down(bool blocked);

Q_SIGNALS:
  void wrapped(int direction);

private:
  bool m_block_wrap_up = false;
  bool m_block_wrap_down = false;
};
