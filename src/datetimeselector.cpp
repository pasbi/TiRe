#include "datetimeselector.h"
#include "fmt.h"

#include <QCheckBox>
#include <QKeyEvent>
#include <QPainter>
#include <spdlog/spdlog.h>

DateTimeSelector::DateTimeSelector(QWidget* parent) : QWidget(parent)
{
  setFocusPolicy(Qt::StrongFocus);
}

void DateTimeSelector::go_left()
{
  set_left_fence(m_left_fence - advance());
}

void DateTimeSelector::go_right()
{
  set_left_fence(m_left_fence + advance());
}

QDateTime DateTimeSelector::date_time() const
{
  return m_current_date_time;
}

void DateTimeSelector::set_date_time(const QDateTime& date_time)
{
  if (m_current_date_time == date_time) {
    return;
  }

  m_current_date_time = constrain(date_time);
  if (m_current_date_time < m_left_fence || m_current_date_time > m_right_fence) {
    set_left_fence(m_current_date_time - visible_range() / 2);
  }

  Q_EMIT date_time_changed(m_current_date_time);
  update();
}

void DateTimeSelector::set_minimum_date_time(const QDateTime& minimum_date_time)
{
  m_minimum_date_time = minimum_date_time;
  set_left_fence(m_left_fence);
  update();
}

void DateTimeSelector::set_maximum_date_time(const QDateTime& maximum_date_time)
{
  m_maximum_date_time = maximum_date_time;
  set_left_fence(m_left_fence);
  update();
}

void DateTimeSelector::paintEvent(QPaintEvent* const event)
{
  QPainter painter(this);

  QPen pen;
  pen.setColor(palette().color(QPalette::Text));
  const auto mid_y = static_cast<double>(height()) / 2.0;

  painter.drawLine(QPointF(0, mid_y), QPointF(width(), mid_y));
  using std::chrono_literals::operator""min;
  using std::chrono_literals::operator""h;
  const auto height_5 = height() * 0.6;
  const auto height_15 = height() * 0.8;
  painter.drawText(QPointF(0, height() - painter.fontMetrics().descent()), m_left_fence.date().toString());
  const auto draw_tick = [&painter, mid_y](const double x, const double height) {
    painter.drawLine(QPointF(x, mid_y + height / 2.0), QPointF(x, mid_y - height / 2.0));
  };
  const auto min_per_px = std::chrono::duration<double>(visible_range()) / static_cast<double>(width());
  for (auto d = QDateTime{m_left_fence.date(), QTime{m_left_fence.time().hour(), 0}}; d < m_right_fence + 1h; d += 1h) {
    const auto color = QPalette().color(d.time().hour() % 2 ? QPalette::Base : QPalette::AlternateBase);
    const QPointF tl{std::chrono::duration<double>(d - m_left_fence) / min_per_px, mid_y - height_15 / 2.0};
    const QSizeF size{static_cast<double>(1.0h / min_per_px), height_15};
    painter.fillRect({tl, size}, color);
  }
  for (auto d = 0min; d < visible_range(); ++d) {
    const auto x = std::chrono::duration<double>(d) / min_per_px;
    const auto date_time = m_left_fence + d;
    if (date_time.time().minute() % 30 == 0) {
      painter.drawText(QPointF(x, mid_y), date_time.time().toString("hh:mm"));
    }
    if (date_time.time().minute() % 15 == 0) {
      draw_tick(x, height_15);
    } else if (date_time.time().minute() % 5 == 0) {
      draw_tick(x, height_5);
    }
  }
  pen.setColor(palette().color(QPalette::Highlight));
  pen.setWidth(3);
  painter.setPen(pen);
  draw_tick((m_current_date_time - m_left_fence) / min_per_px, height_15);
}

void DateTimeSelector::mouseMoveEvent(QMouseEvent* const event)
{
  if (m_mouse_press_state.has_value()) {
    const auto d = m_mouse_press_state->pos - event->pos();
    set_left_fence(m_mouse_press_state->left_fence + px_to_min(d.x()));
  }
  QWidget::mouseMoveEvent(event);
  update();
}

void DateTimeSelector::mousePressEvent(QMouseEvent* const event)
{
  m_mouse_press_state = {.pos = event->pos(), .left_fence = m_left_fence};
  if (!(event->modifiers() & Qt::ControlModifier)) {
    set_date_time(m_left_fence + px_to_min(event->pos().x()));
  }
  QWidget::mousePressEvent(event);
}

void DateTimeSelector::mouseReleaseEvent(QMouseEvent* const event)
{
  m_mouse_press_state.reset();
  QWidget::mouseReleaseEvent(event);
}

void DateTimeSelector::keyPressEvent(QKeyEvent* const event)
{
  if (event->key() == Qt::Key_Escape && m_mouse_press_state.has_value()) {
    set_left_fence(m_mouse_press_state->left_fence);
    m_mouse_press_state.reset();
    event->accept();
    return;
  }
  QWidget::keyPressEvent(event);
}

void DateTimeSelector::resizeEvent(QResizeEvent* const event)
{
  set_left_fence(m_left_fence);
  QWidget::resizeEvent(event);
}

void DateTimeSelector::wheelEvent(QWheelEvent* const event)
{
  using std::chrono_literals::operator""min;
  m_accumulated_wheel_y += event->angleDelta().y();
  static constexpr auto threshold = 120;
  if (std::abs(m_accumulated_wheel_y) < threshold) {
    return;
  }
  const auto advance = 1min * m_accumulated_wheel_y / threshold;
  m_accumulated_wheel_y = m_accumulated_wheel_y % threshold;
  set_date_time(m_current_date_time + advance);
}

std::chrono::minutes DateTimeSelector::visible_range() const
{
  return px_to_min(width());
}

std::chrono::minutes DateTimeSelector::advance() const
{
  return visible_range() / 2;
}

void DateTimeSelector::set_left_fence(const QDateTime& left_fence)
{
  using std::operator""min;
  static constexpr auto overshoot = 15min;
  m_left_fence = constrain(left_fence, overshoot);
  m_right_fence = constrain(m_left_fence + visible_range(), overshoot);

  Q_EMIT left_fence_reached_changed(m_left_fence <= m_minimum_date_time);
  Q_EMIT right_fence_reached_changed(m_right_fence >= m_maximum_date_time);

  update();
}

std::chrono::minutes DateTimeSelector::px_to_min(const int px)
{
  static constexpr auto pixel_per_minute = 3;
  using std::chrono_literals::operator""min;
  return px / pixel_per_minute * 1min;
}

QDateTime DateTimeSelector::constrain(QDateTime date_time, const std::chrono::minutes& overshoot) const
{
  if (m_maximum_date_time.isValid()) {
    date_time = std::min(m_maximum_date_time + overshoot, date_time);
  }
  if (m_minimum_date_time.isValid()) {
    date_time = std::max(m_minimum_date_time - overshoot, date_time);
  }
  return date_time;
};
