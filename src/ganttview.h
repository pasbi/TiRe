#pragma once

#include "period.h"
#include <QWidget>

class TimeSheet;
class Interval;

class GanttView : public QWidget
{
  Q_OBJECT
public:
  explicit GanttView(QWidget* parent = nullptr);
  void set_time_sheet(const TimeSheet* time_sheet);
  void set_current_interval(const Interval* interval);
  void select_period(const Period& period);
  void ensure_visible(const Period& period);

protected:
  void paintEvent(QPaintEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;

Q_SIGNALS:
  void clicked(QDateTime date);

private:
  const TimeSheet* m_time_sheet = nullptr;
  const Interval* m_current_interval = nullptr;
  Period m_period;
  Period m_selected_period;

  [[nodiscard]] double pos_y(const QDate& date) const;
  [[nodiscard]] QDate date_at(double y) const;
  [[nodiscard]] double pos_x(const QTime& time) const;
  [[nodiscard]] QTime time_at(double x) const;
  [[nodiscard]] QDateTime datetime_at(const QPointF& pos) const;
  [[nodiscard]] double day_height() const;
  [[nodiscard]] std::vector<QRectF> rects(const Interval& interval) const;
  void draw_grid(QPainter& painter) const;
  [[nodiscard]] QRectF rect(const QDate& date, const QTime& begin, const QTime& end) const;
  [[nodiscard]] QRectF rect(const QDate& date) const;

  [[nodiscard]] QColor interpolate_base(double t) const noexcept;
};
