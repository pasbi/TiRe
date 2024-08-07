#include "colorutil.h"
#include <QApplication>
#include <QDate>
#include <QPalette>

QColor mix_base(const double t, const QColor& other)
{
  const auto palette = QApplication::palette();
  const auto base = palette.base().color();

  return QColor(std::lerp(base.red(), other.red(), t), std::lerp(base.green(), other.green(), t),
                std::lerp(base.blue(), other.blue(), t), std::lerp(base.alpha(), other.alpha(), t));
}

QColor contrast_color(const QColor& color)
{
  const auto is_bright = color.lightnessF() > 0.5;
  return QColor(is_bright ? Qt::black : Qt::white);
}

QColor background(const QDate& date)
{
  const auto d = date.dayOfWeek();
  const auto color_factor = d == Qt::Sunday || d == Qt::Saturday ? 0.2 : 0.0;
  return ::mix_base(color_factor, Qt::red);
}
