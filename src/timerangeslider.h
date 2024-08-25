#pragma once

#include <QWidget>

class TimeRangeSlider final : public QWidget
{
public:
  explicit TimeRangeSlider(QWidget* parent = nullptr);
  [[nodiscard]] QTime begin() const;
  [[nodiscard]] QTime end() const;

protected:
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void paintEvent(QPaintEvent* event) override;

private:
  double m_min = 0.0;
  double m_max = 24.0 * 60.0;
  double m_begin = 0.0;
  double m_end = 0.0;
  bool m_active = false;

  [[nodiscard]] double to_pixel(const double value) const;
  [[nodiscard]] double from_pixel(const double px) const;
  void draw_minute_tick(QPainter& painter, int minute, double mid_y) const;
  void draw_hour_tick(QPainter& painter, int hour, double mid_y) const;
};
