#include "timeeditspinbox.h"

void TimeEditSpinBox::stepBy(const int steps)
{
  const auto value_before = value();
  QSpinBox::stepBy(steps);

  if (const auto value_after = value(); value_after > value_before && steps < 0) {
    if (m_block_wrap_down) {
      setValue(minimum());
    } else {
      Q_EMIT wrapped(-1);
    }
  } else if (value_after < value_before && steps > 0) {
    if (m_block_wrap_up) {
      setValue(maximum());
    } else {
      Q_EMIT wrapped(1);
    }
  }
}

QString TimeEditSpinBox::textFromValue(const int value) const
{
  return QString{"%1"}.arg(value, 2, 10, QChar('0'));
}

void TimeEditSpinBox::set_block_wrap_up(const bool blocked)
{
  m_block_wrap_up = blocked;
}

void TimeEditSpinBox::set_block_wrap_down(const bool blocked)
{
  m_block_wrap_down = blocked;
}
