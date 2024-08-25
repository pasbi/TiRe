#pragma once

#include <QWidget>

class TimeRangeSlider final : public QWidget
{
  Q_OBJECT
public:
  explicit TimeRangeSlider(QWidget* parent = nullptr);
  [[nodiscard]] QTime begin() const;
  [[nodiscard]] QTime mid() const;
  [[nodiscard]] QTime end() const;

  void set_begin(const QTime& begin);
  void set_mid(const QTime& mid);
  void set_end(const QTime& end);

  void set_split(bool split);

Q_SIGNALS:
  void begin_changed(const QTime& begin);
  void end_changed(const QTime& begin);
  void mid_changed(const QTime& split);

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
  double m_mid = 0.0;
  bool m_active = false;
  bool m_allow_begin_after_end = false;
  bool m_split = false;

  [[nodiscard]] double to_pixel(const double value) const;
  [[nodiscard]] double from_pixel(const double px) const;
  void draw_minute_tick(QPainter& painter, int minute, double mid_y) const;
  void draw_hour_tick(QPainter& painter, int hour, double mid_y) const;
};
