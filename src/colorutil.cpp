#include "colorutil.h"
#include <QApplication>
#include <QDate>
#include <QPalette>
#include <cmath>

namespace
{
/** @brief Interpolates one 0-255 color channel, rounding rather than truncating. */
[[nodiscard]] int lerp_channel(const int a, const int b, const double t)
{
  return static_cast<int>(std::lround(std::lerp(a, b, t)));
}
}  // namespace

QColor lerp(const double t, const QColor& a, const QColor& b)
{
  return {::lerp_channel(a.red(), b.red(), t), ::lerp_channel(a.green(), b.green(), t),
          ::lerp_channel(a.blue(), b.blue(), t), ::lerp_channel(a.alpha(), b.alpha(), t)};
}

QColor contrast_color(const QColor& color)
{
  const auto is_bright = color.lightnessF() > 0.5;
  return {is_bright ? Qt::black : Qt::white};
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
