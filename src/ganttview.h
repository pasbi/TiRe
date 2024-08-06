#pragma once
#include "intervalmodel.h"

#include <QWidget>

class GanttView : public QWidget
{
public:
  explicit GanttView(QWidget* parent = nullptr);
  void set_model(const IntervalModel* interval_model);

protected:
  void paintEvent(QPaintEvent* event) override;

private:
  const IntervalModel* m_interval_model = nullptr;
  Period m_period;

  [[nodiscard]] double pos_y(const QDate& date) const;
  [[nodiscard]] double pos_x(const QTime& time) const;
  [[nodiscard]] QPointF pos(const QDateTime& date_time) const;
  [[nodiscard]] double day_height() const;
  [[nodiscard]] std::vector<QRectF> rects(const Interval& interval) const;
  void draw_grid(QPainter& painter) const;
  [[nodiscard]] QRectF rect(const QDate& date, const QTime& begin, const QTime& end) const;

  [[nodiscard]] QColor interpolate_base(const double t) const noexcept;
};
