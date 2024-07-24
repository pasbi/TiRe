#include "leadingzerosspinbox.h"

QString LeadingZerosSpinBox::textFromValue(const int val) const
{
  static constexpr auto field_width = 2;
  static constexpr auto base = 10;
  static constexpr auto fill_char = QChar{'0'};
  return QStringLiteral("%1").arg(val, field_width, base, fill_char);
}