#include "ganttview.h"

#include "colorutil.h"
#include "interval.h"
#include "period.h"
#include <QDateTime>
#include <QPainter>

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
  : QWidget(parent), m_period(QDate::currentDate().addDays(-default_gantt_length_days), QDate::currentDate())
{
}

void GanttView::set_model(const IntervalModel* interval_model)
{
  m_interval_model = interval_model;
  if (m_interval_model != nullptr) {
    connect(m_interval_model, &IntervalModel::data_changed, this, QOverload<>::of(&QWidget::update));
  }
  update();
}

void GanttView::paintEvent(QPaintEvent* event)
{
  if (m_interval_model == nullptr) {
    return;
  }

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  for (int day = 0; day < m_period.days(); ++day) {
    const auto date = m_period.begin().addDays(day);
    if (const auto d = date.dayOfWeek(); d == Qt::Saturday || d == Qt::Sunday) {
      const auto rect = this->rect(date, date.startOfDay().time(), date.endOfDay().time());
      painter.fillRect(rect, ::background(date));
    }
  }

  for (const auto* const interval : m_interval_model->intervals()) {
    for (const auto& rect : rects(*interval)) {
      painter.fillRect(rect, interval->project().color());
    }
  }

  draw_grid(painter);
}

double GanttView::pos_y(const QDate& date) const
{
  return static_cast<double>(m_period.begin().daysTo(date))
         / static_cast<double>(m_period.begin().daysTo(m_period.end()) + 1) * static_cast<double>(height());
}

double GanttView::pos_x(const QTime& time) const
{
  using std::chrono_literals::operator""h;
  using std::chrono_literals::operator""min;
  return (static_cast<double>(time.hour()) * 1.0h + static_cast<double>(time.minute()) * 1.0min) / 24.0h
         * static_cast<double>(width());
}

std::vector<QRectF> GanttView::rects(const Interval& interval) const
{
  const auto& begin = interval.begin();
  const auto end = interval.end().isValid() ? interval.end() : QDateTime::currentDateTime();
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
  painter.setPen(::mix_base(0.1, text_color));
  for (auto hour = 0h; hour <= 24h; ++hour) {
    const auto x = pos_x(QTime{static_cast<int>(hour / 1h), 0});
    painter.drawLine(QPointF{x, 0.0}, QPointF{x, static_cast<double>(height())});
  }
  painter.setPen(::mix_base(0.9, text_color));
  for (auto day = 0; day <= m_period.days(); ++day) {
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
