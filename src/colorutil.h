#pragma once

#include <QColor>

class QDate;

[[nodiscard]] QColor lerp(double t, const QColor& a, const QColor& b);
[[nodiscard]] QColor contrast_color(const QColor& color);
[[nodiscard]] QColor background(const QDate& date);
[[nodiscard]] QColor selected(const QColor& color);
