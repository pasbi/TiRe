#pragma once

#include <QColor>

[[nodiscard]] QColor mix_base(const double t, const QColor& other);
[[nodiscard]] QColor contrast_color(const QColor& color);
