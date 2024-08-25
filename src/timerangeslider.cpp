#include "timerangeslider.h"
#include <QMouseEvent>
#include <QPainter>
#include <QTime>
#include <QToolTip>
#include <ranges>

namespace
{

[[nodiscard]] QTime to_time(const int minutes)
{
  return {(minutes / 60) % 24, minutes % 60};
}

[[nodiscard]] int to_minutes(const QTime time)
{
  return time.hour() * 60 + time.minute();
}

void draw_vertical_line(QPainter& painter, const double x, const double top, const double bottom)
{
  painter.drawLine(QPointF{x, top}, QPointF{x, bottom});
}

}  // namespace

TimeRangeSlider::TimeRangeSlider(QWidget* parent) : QWidget(parent)
{
  setMouseTracking(true);
  setMinimumWidth(static_cast<int>(m_max - m_min));
}

void TimeRangeSlider::mousePressEvent(QMouseEvent* event)
{
  const auto time = ::to_time(static_cast<int>(from_pixel(event->pos().x())));
  if (event->modifiers() == Qt::ControlModifier) {
    m_active = true;
    set_begin(time);
    set_end(time);
  } else if (event->button() == Qt::LeftButton) {
    set_begin(time);
  } else if (event->button() == Qt::RightButton) {
    set_end(time);
  }
  update();
}

void TimeRangeSlider::mouseMoveEvent(QMouseEvent* event)
{
  static constexpr auto format = [](const QTime& time) { return time.toString("hh:mm"); };
  if (m_active) {
    set_end(::to_time(from_pixel(event->pos().x())));
    QToolTip::showText(event->globalPosition().toPoint(), tr("%1 - %2").arg(format(begin()), format(end())));
  } else {
    const auto x = from_pixel(event->pos().x());
    const auto time = ::to_time(static_cast<int>(x));
    QToolTip::showText(event->globalPosition().toPoint(), tr("%1").arg(format(time)));
  }
  update();
}

void TimeRangeSlider::mouseReleaseEvent(QMouseEvent* event)
{
  if (m_active) {
    set_end(::to_time(from_pixel(event->pos().x())));
    m_active = false;
    update();
  }
  if (m_begin > m_end) {
    const auto tmp = m_end;
    set_end(::to_time(m_begin));
    set_begin(::to_time(tmp));
  }
}

void TimeRangeSlider::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);

  QPen pen;
  pen.setColor(palette().color(QPalette::Text));
  const auto mid_y = static_cast<double>(height()) / 2.0;

  const auto dy = 0.5 * height();
  painter.fillRect(QRectF{QPointF{to_pixel(m_begin), mid_y - dy}, QPointF{to_pixel(m_end), mid_y + dy}},
                   palette().color(QPalette::Highlight));
  pen.setWidthF(10.0);
  painter.setPen(pen);
  painter.drawLine(QPointF{to_pixel(m_min), mid_y}, QPointF(to_pixel(m_max), mid_y));

  auto font = painter.font();
  font.setBold(true);
  font.setPointSizeF(12.0);
  painter.setFont(font);
  for (int i = 0; i <= 24 * 60; ++i) {
    if (i % 60 == 0) {
      draw_hour_tick(painter, i, mid_y);
    } else if (i % 15 == 0) {
      draw_minute_tick(painter, i, mid_y);
    }
  }
}

QTime TimeRangeSlider::begin() const
{
  return ::to_time(static_cast<int>(m_begin));
}

QTime TimeRangeSlider::end() const
{
  return ::to_time(static_cast<int>(m_end));
}

void TimeRangeSlider::set_begin(const QTime& begin)
{
  if (const auto minutes = ::to_minutes(begin); minutes != m_begin) {
    m_begin = minutes;
    Q_EMIT begin_changed(begin);
  }
}

void TimeRangeSlider::set_end(const QTime& end)
{
  if (const auto minutes = ::to_minutes(end); minutes != m_end) {
    m_end = minutes;
    Q_EMIT end_changed(end);
  }
}

void TimeRangeSlider::set_allow_begin_after_end(const bool allow)
{
  m_allow_begin_after_end = allow;
}

double TimeRangeSlider::to_pixel(const double value) const
{
  return (value - m_min) / (m_max - m_min) * static_cast<double>(width());
}

double TimeRangeSlider::from_pixel(const double px) const
{
  return px / static_cast<double>(width()) * (m_max - m_min) + m_min;
}

void TimeRangeSlider::draw_hour_tick(QPainter& painter, const int hour, const double mid_y) const
{
  auto pen = painter.pen();
  pen.setWidthF(2.0);
  painter.setPen(pen);
  const auto pos_x = to_pixel(hour);
  const auto dy = 0.5 * height();
  const auto top = mid_y - dy;
  const auto bottom = mid_y + dy;
  draw_vertical_line(painter, pos_x, top, bottom);
  static constexpr auto field_width = 2;
  static constexpr auto base = 10;
  static constexpr auto fill_char = QChar('0');
  painter.drawText(static_cast<int>(pos_x) + 2, static_cast<int>(top) + painter.fontMetrics().height(),
                   QStringLiteral("%1").arg(hour / 60, field_width, base, fill_char));
}

void TimeRangeSlider::draw_minute_tick(QPainter& painter, const int minute, const double mid_y) const
{
  auto pen = painter.pen();
  pen.setWidthF(1.0);
  painter.setPen(pen);
  const auto dy = 0.3 * height();
  draw_vertical_line(painter, to_pixel(minute), mid_y - dy, mid_y + dy);
}
