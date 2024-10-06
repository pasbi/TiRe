#include "colorutil.h"
#include <QApplication>
#include <QDate>
#include <QPalette>

QColor lerp(const double t, const QColor& a, const QColor& b)
{
  return QColor(std::lerp(a.red(), b.red(), t), std::lerp(a.green(), b.green(), t), std::lerp(a.blue(), b.blue(), t),
                std::lerp(a.alpha(), b.alpha(), t));
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
  return ::lerp(color_factor, QApplication::palette().base().color(), Qt::red);
}

QColor selected(const QColor& color)
{
  return ::lerp(0.2, color, QApplication::palette().highlight().color());
}
