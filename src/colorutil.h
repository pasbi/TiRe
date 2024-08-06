#pragma once

#include <QColor>

class QDate;

[[nodiscard]] QColor mix_base(double t, const QColor& other);
[[nodiscard]] QColor contrast_color(const QColor& color);
[[nodiscard]] QColor background(const QDate& date);
