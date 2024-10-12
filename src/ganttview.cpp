#include "ganttview.h"
#include "application.h"
#include "colorutil.h"
#include "interval.h"
#include "intervalmodel.h"
#include "period.h"
#include "plan.h"
#include "timesheet.h"

#include <QDateTime>
#include <QHelpEvent>
#include <QPainter>
#include <QToolTip>

namespace
{

class PainterSaver
{
public:
  explicit PainterSaver(QPainter& painter) : m_painter(painter)
  {
    m_painter.save();
  }

  ~PainterSaver()
  {
    m_painter.restore();
  }

private:
  QPainter& m_painter;
};

constexpr auto default_gantt_length_days = 30;

}  // namespace

GanttView::GanttView(QWidget* parent)
  : QWidget(parent)
  , m_period(Application::current_date_time().date().addDays(-default_gantt_length_days),
             Application::current_date_time().date())
{
  setMouseTracking(true);
}

void GanttView::set_time_sheet(const TimeSheet* time_sheet)
{
  m_time_sheet = time_sheet;
  m_current_interval = nullptr;
  if (m_time_sheet != nullptr) {
    connect(&m_time_sheet->interval_model(), &IntervalModel::data_changed, this, QOverload<>::of(&QWidget::update));
  }
  update();
}

void GanttView::set_current_interval(const Interval* interval)
{
  m_current_interval = interval;
  update();
}

void GanttView::select_period(const Period& period)
{
  m_selected_period = period;
  update();
}

void GanttView::paintEvent(QPaintEvent* event)
{
  if (m_time_sheet == nullptr) {
    return;
  }

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  for (int day = 0; day < m_period.days(); ++day) {
    const auto date = m_period.begin().addDays(day);
    auto bg_color = ::background(date);
    if (m_selected_period.contains(date)) {
      bg_color = ::selected(bg_color);
    }
    const auto rect = this->rect(date, date.startOfDay().time(), date.endOfDay().time());
    painter.fillRect(rect, bg_color);
  }

  for (const auto* const interval : m_time_sheet->interval_model().intervals()) {
    for (const auto& rect : rects(*interval)) {
      if (const auto* const project = interval->project()) {
        painter.fillRect(rect, project->color());
      }
    }
  }

  draw_grid(painter);
  if (m_current_interval != nullptr) {
    auto pen = painter.pen();
    pen.setWidthF(2.0);
    pen.setCosmetic(true);
    pen.setColor(Qt::black);
    painter.setPen(pen);
    for (const auto& rect : rects(*m_current_interval)) {
      painter.drawRect(rect);
    }
  }
}

void GanttView::mouseMoveEvent(QMouseEvent* event)
{
  const auto date_time = datetime_at(event->pos());
  const auto kind_of_day = m_time_sheet->plan().find_kind(date_time.date());
  QToolTip::showText(
      event->globalPosition().toPoint(),
      QString::fromStdString(fmt::format("{}: {}", date_time.toString("dddd, dd.MM. hh:mm"), kind_of_day)));
}

void GanttView::mousePressEvent(QMouseEvent* const event)
{
  Q_EMIT clicked(datetime_at(event->pos()));
}

double GanttView::pos_y(const QDate& date) const
{
  return static_cast<double>(m_period.begin().daysTo(date)) / static_cast<double>(m_period.days())
         * static_cast<double>(height());
}

QDate GanttView::date_at(const double y) const
{
  return m_period.begin().addDays(static_cast<int>(m_period.days() * y / static_cast<double>(height())));
}

double GanttView::pos_x(const QTime& time) const
{
  using std::chrono_literals::operator""h;
  using std::chrono_literals::operator""min;
  return (static_cast<double>(time.hour()) * 1.0h + static_cast<double>(time.minute()) * 1.0min) / 24.0h
         * static_cast<double>(width());
}

QTime GanttView::time_at(const double x) const
{
  using std::chrono_literals::operator""h;
  using std::chrono_literals::operator""min;
  const auto total = 24.0h * x / static_cast<double>(width());
  return {static_cast<int>(total / 60min), static_cast<int>(total / 1min) % 60};
}

QDateTime GanttView::datetime_at(const QPointF& pos) const
{
  return {date_at(pos.y()), time_at(pos.x())};
}

std::vector<QRectF> GanttView::rects(const Interval& interval) const
{
  const auto& begin = interval.begin();
  const auto end = interval.end().isValid() ? interval.end() : Application::current_date_time();
  const auto day_count = begin.date().daysTo(end.date());
  std::vector<QRectF> rects;
  rects.reserve(day_count);
  for (int day = 0; day <= day_count; ++day) {
    const auto current_day = begin.date().addDays(day);
    rects.emplace_back(rect(current_day, std::max(begin, current_day.startOfDay()).time(),
                            std::min(end, current_day.endOfDay()).time()));
  }
  return rects;
}

void GanttView::draw_grid(QPainter& painter) const
{
  const PainterSaver _(painter);
  using std::chrono_literals::operator""h;
  const auto text_color = palette().text().color();
  const auto base_color = palette().base().color();
  painter.setPen(::lerp(0.1, text_color, base_color));
  for (auto hour = 0h; hour <= 24h; ++hour) {
    const auto x = pos_x(QTime{static_cast<int>(hour / 1h), 0});
    painter.drawLine(QPointF{x, 0.0}, QPointF{x, static_cast<double>(height())});
  }
  painter.setPen(::lerp(0.9, text_color, base_color));
  for (auto day = 0; day < m_period.days(); ++day) {
    const auto y = pos_y(m_period.begin().addDays(day));
    painter.drawLine(QPointF{0.0, y}, QPointF{static_cast<double>(width()), y});
  }
}

QRectF GanttView::rect(const QDate& date, const QTime& begin, const QTime& end) const
{
  const auto pos_x_begin = pos_x(begin);
  const auto pos_x_end = pos_x(end);
  const auto pos_y_begin = pos_y(date);
  const auto pos_y_end = pos_y(date.addDays(1));
  return {QPointF{pos_x_begin, pos_y_begin}, QPointF{pos_x_end, pos_y_end}};
}
