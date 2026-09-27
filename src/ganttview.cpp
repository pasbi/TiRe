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
#include <algorithm>
#include <optional>
#include <ranges>

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

/** @brief The kind that decides a day's hatching: the one claiming most of it. */
[[nodiscard]] std::optional<Plan::Kind> dominant_kind(const Plan::DayShares& shares)
{
  using enum Plan::Kind;
  // Listed rather than generated, because the order is the tie-breaker: max_element returns the
  // first of equal shares, so half a sick day next to half a vacation day reads as sick.
  static constexpr auto kinds = std::array{Sick, ChildSick, Holiday, Vacation};
  static_assert(kinds.size() == Plan::kind_count, "every kind must be able to win");
  const auto share_of = [&shares](const auto kind) { return shares[kind]; };
  const auto dominant = *std::ranges::max_element(kinds, std::less<>{}, share_of);
  if (shares[dominant] == Plan::Share::none()) {
    return {};
  }
  return dominant;
}

Qt::BrushStyle brush_style(const Plan::DayShares& shares)
{
  const auto kind = ::dominant_kind(shares);
  if (!kind.has_value()) {
    return Qt::NoBrush;
  }
  // The pattern names the leave, its density says whether the day is off entirely -- so half a
  // bank holiday topped up with half a vacation day still reads as a whole day off, which is
  // what it is.
  const auto whole_day = shares.budget() + shares.waiver() >= Plan::Share::whole();
  switch (*kind) {
    using enum Plan::Kind;
  case Sick:
    return whole_day ? Qt::DiagCrossPattern : Qt::FDiagPattern;
  case ChildSick:
    return whole_day ? Qt::Dense4Pattern : Qt::Dense6Pattern;
  case Holiday:
  case Vacation:
    return whole_day ? Qt::CrossPattern : Qt::VerPattern;
  }
  Q_UNREACHABLE();
}

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

  for (const auto& date : m_period.dates()) {
    auto bg_color = ::background(date);
    if (m_selected_period.contains(date)) {
      bg_color = ::selected(bg_color);
    }
    painter.fillRect(rect(date), bg_color);
  }

  for (const auto* const interval : m_time_sheet->interval_model().intervals()) {
    for (const auto& rect : rects(*interval)) {
      if (const auto* const project = interval->project()) {
        painter.fillRect(rect, project->color());
      }
    }
  }

  // Asked for in one go rather than per day: every lookup walks all plan entries.
  const auto shares = m_time_sheet->plan().shares_in(m_period);
  for (const auto i : std::views::iota(0uz, shares.size())) {
    painter.fillRect(rect(m_period.begin().addDays(static_cast<qint64>(i))), ::brush_style(shares.at(i)));
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

  painter.setPen([this] {
    QPen pen;
    pen.setCosmetic(true);
    pen.setColor(palette().text().color());
    return pen;
  }());

  const auto display_date = [this](const QDate& date) {
    if (m_period.begin().weekNumber() != m_period.end().weekNumber() || m_period.begin().dayOfWeek() == Qt::Monday) {
      return date.dayOfWeek() == Qt::Monday;
    }
    return false;
  };

  for (const auto& date : m_period.dates()) {
    if (display_date(date)) {
      painter.drawText(rect(date).bottomLeft(), date.toString("dddd, dd.MM."));
    }
  }
}

void GanttView::mouseMoveEvent(QMouseEvent* event)
{
  const auto date_time = datetime_at(event->pos());
  const auto shares_of_day = m_time_sheet->plan().shares_on(date_time.date());
  const auto shares_of_day_text = shares_of_day.empty() ? "" : fmt::format(" [{}]", shares_of_day);
  QToolTip::showText(event->globalPosition().toPoint(),
                     date_time.toString("dddd, dd.MM. hh:mm") + QString::fromStdString(shares_of_day_text), this);
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
  // The chrono literals are long double-backed, so the ratio is narrowed to double explicitly.
  const auto fraction_of_day = static_cast<double>(
      (static_cast<double>(time.hour()) * 1.0h + static_cast<double>(time.minute()) * 1.0min) / 24.0h);
  return fraction_of_day * static_cast<double>(width());
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
  painter.setPen(::lerp(0.8, text_color, base_color));
  for (auto hour = 0h; hour <= 24h; ++hour) {
    const auto x = pos_x(QTime{static_cast<int>(hour / 1h), 0});
    painter.drawLine(QPointF{x, 0.0}, QPointF{x, static_cast<double>(height())});
  }
  painter.setPen(::lerp(0.9, text_color, base_color));
  for (const auto& date : m_period.dates()) {
    const auto y = pos_y(date);
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

QRectF GanttView::rect(const QDate& date) const
{
  return rect(date, date.startOfDay().time(), date.endOfDay().time());
}

void GanttView::ensure_visible(const Period& period)
{
  if (const auto fill = default_gantt_length_days - period.days(); fill > 0) {
    const auto fill_end = fill / 4;
    const auto begin = period.begin().addDays(fill_end - fill);
    const auto end = period.end().addDays(fill_end);
    m_period = Period{begin, end};
  } else {
    m_period = period;
  }
  update();
}
