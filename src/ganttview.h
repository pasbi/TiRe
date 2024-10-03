#pragma once

#include "intervalmodel.h"
#include <QWidget>

class GanttView : public QWidget
{
  Q_OBJECT
public:
  explicit GanttView(QWidget* parent = nullptr);
  void set_model(const IntervalModel* interval_model);
  void set_current_interval(const Interval* interval);

protected:
  void paintEvent(QPaintEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;

Q_SIGNALS:
  void clicked(QDateTime date);

private:
  const IntervalModel* m_interval_model = nullptr;
  const Interval* m_current_interval = nullptr;
  Period m_period;

  [[nodiscard]] double pos_y(const QDate& date) const;
  [[nodiscard]] QDate date_at(double y) const;
  [[nodiscard]] double pos_x(const QTime& time) const;
  [[nodiscard]] QTime time_at(double x) const;
  [[nodiscard]] QDateTime datetime_at(const QPointF& pos) const;
  [[nodiscard]] double day_height() const;
  [[nodiscard]] std::vector<QRectF> rects(const Interval& interval) const;
  void draw_grid(QPainter& painter) const;
  [[nodiscard]] QRectF rect(const QDate& date, const QTime& begin, const QTime& end) const;

  [[nodiscard]] QColor interpolate_base(const double t) const noexcept;
};
