#include "timeedit.h"
#include "fmt.h"

#include <QPainterPath>

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>

class TimeEdit::Item
{
public:
  explicit Item(const QTransform& transform);
  virtual ~Item() = default;
  virtual void paint(QPainter& painter) = 0;
  [[nodiscard]] const QTransform& transform() const noexcept;

private:
  QRect m_bounding_box;
  const QTransform& m_transform;
};

class TimeEdit::NumberItem final : public Item
{
public:
  enum class Type { Hour, Minute };
  explicit NumberItem(const QTransform& transform, const bool interactive, const double number, const Type type)
    : Item(transform), m_number(number), m_type(type), m_interactive(interactive)
  {
  }

  void paint(QPainter& painter) override
  {
    const auto rel_pos = compute_rel_pos();

    if (m_interactive) {
      auto pen = painter.pen();
      pen.setWidthF(2.0);
      painter.setPen(pen);
      painter.drawLine(transform().map(QLineF{rel_pos, QPointF{0.0, 0.0}}));
    }

    const double size = m_interactive ? 0.35 : 0.3;
    const auto box = transform().map(QRectF{rel_pos - QPointF{size, size} / 2.0, QSizeF{size, size}}).boundingRect();
    QPainterPath shape;
    shape.addEllipse(box);
    painter.fillPath(shape, m_active ? QApplication::palette().accent() : QApplication::palette().alternateBase());
    painter.drawPath(shape);
    painter.drawText(box, Qt::AlignCenter,
                     QStringLiteral("%1").arg(static_cast<int>(std::round(m_number)), m_type == Type::Hour ? 1 : 2));
  }

  void set_number(const double number) noexcept
  {
    const auto max = static_cast<double>(this->max());
    m_number = std::fmod(std::fmod(number, max) + max, max);  // ensure number is in [0, max())
    if (m_type == Type::Hour && m_number < 1.0) {
      m_number += max;  // for hour, we want 1-12
    }
  }

  [[nodiscard]] int number() const
  {
    return std::floor(m_number);
  }

  void move_to(const QPointF& new_pos)
  {
    const auto rel_pos = transform().inverted().map(new_pos);
    set_number(compute_number(rel_pos));
  }

  [[nodiscard]] QTransform transform() const
  {
    auto t = Item::transform();
    t.scale(radius(), radius());
    return t;
  }

  void set_active(const bool active) noexcept
  {
    m_active = active;
  }

  void quantify()
  {
    set_number(std::round(m_number));
  }

  [[nodiscard]] int max() const noexcept
  {
    switch (m_type) {
    case Type::Hour:
      return 12;
    case Type::Minute:
      return 60;
    }
    return 0;
  }

  [[nodiscard]] double radius() const noexcept
  {
    switch (m_type) {
    case Type::Hour:
      return 0.5;
    case Type::Minute:
      return 0.8;
    }
    return 0;
  }

private:
  double m_number;
  const Type m_type;
  bool m_active = false;
  bool m_interactive = false;
  [[nodiscard]] double compute_number(const QPointF& rel_pos) const noexcept
  {
    return (std::atan2(rel_pos.y(), rel_pos.x()) + M_PI_2) / M_PI / 2.0 * static_cast<double>(max());
  }

  [[nodiscard]] QPointF compute_rel_pos() const noexcept
  {
    const auto angle = 2.0 * M_PI * m_number / static_cast<double>(max()) - M_PI_2;
    return {std::cos(angle), std::sin(angle)};
  }
};

class TimeEdit::AMPMItem final : public Item
{
public:
  enum class State { AM, PM };
  explicit AMPMItem(const QTransform& transform) : Item(transform)
  {
  }

  void paint(QPainter& painter) override
  {
    const auto box = transform().map(QRectF{pos - QPointF{size, size} / 2.0, QSizeF{size, size}}).boundingRect();
    QPainterPath shape;
    shape.addEllipse(box);
    painter.fillPath(shape, QApplication::palette().alternateBase());
    painter.drawPath(shape);
    painter.drawText(box, Qt::AlignCenter, m_state == State::AM ? tr("AM") : tr("PM"));
  }

  [[nodiscard]] State state() const noexcept
  {
    return m_state;
  }

  void set_state(const State state) noexcept
  {
    m_state = state;
  }

  void toggle()
  {
    set_state(state() == State::AM ? State::PM : State::AM);
  }

  static constexpr QPointF pos{0.0, 0.0};
  static constexpr auto size = 0.3;

private:
  State m_state = State::AM;
};

namespace
{
auto make_items(const QTransform& transform, TimeEdit::NumberItem*& hour_item, TimeEdit::NumberItem*& minute_item,
                TimeEdit::AMPMItem*& ampm_item)
{
  std::vector<std::unique_ptr<TimeEdit::Item>> items;
  const auto add_item = [&items, &transform]<typename T, typename... Args>(Args&&... args) -> auto& {
    auto item = std::make_unique<T>(transform, std::forward<Args>(args)...);
    auto& ref = *item;
    items.emplace_back(std::move(item));
    return ref;
  };

  using enum TimeEdit::NumberItem::Type;
  for (int i = 1; i <= 12; ++i) {
    add_item.operator()<TimeEdit::NumberItem>(false, i, Hour);
  }
  for (int i = 0; i < 60; i += 5) {
    add_item.operator()<TimeEdit::NumberItem>(false, i, Minute);
  }
  minute_item = &add_item.operator()<TimeEdit::NumberItem>(true, 0, Minute);
  hour_item = &add_item.operator()<TimeEdit::NumberItem>(true, 0, Hour);
  ampm_item = &add_item.operator()<TimeEdit::AMPMItem>();
  return items;
}

}  // namespace

TimeEdit::TimeEdit(QWidget* const parent)
  : QWidget(parent)
  , m_items(::make_items(m_transform, m_hour_item, m_minute_item, m_ampm_item))
  , m_moveable_items(std::vector{m_hour_item, m_minute_item})
{
}

TimeEdit::~TimeEdit() = default;

QTime TimeEdit::time() const
{
  return m_current_time;
}

void TimeEdit::set_time(const QTime& time)
{
  if (m_current_time == time) {
    return;
  }

  m_current_time = time;
  if (m_update_canvas_mutex.try_lock()) {
    const auto h = m_current_time.hour();
    m_ampm_item->set_state(h < 12 ? AMPMItem::State::AM : AMPMItem::State::PM);
    m_hour_item->set_number((h + 11) % 12 + 1);
    m_minute_item->set_number(m_current_time.minute());
    update();
    m_update_canvas_mutex.unlock();
  }
  Q_EMIT time_changed(m_current_time);
}

QTime TimeEdit::current_time() const
{
  // By convention, 12 AM denotes midnight and 12 PM denotes noon
  const auto h = m_hour_item->number();
  const auto is_am = m_ampm_item->state() == AMPMItem::State::AM;
  return {h == 12 ? (is_am ? 0 : 12) : (is_am ? h : h + 12), m_minute_item->number()};
}

TimeEdit::Item::Item(const QTransform& transform) : m_transform(transform)
{
}

const QTransform& TimeEdit::Item::transform() const noexcept
{
  return m_transform;
}

void TimeEdit::paintEvent(QPaintEvent* const event)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  std::ranges::for_each(m_items, [&painter](const auto& item) { item->paint(painter); });
}

void TimeEdit::mousePressEvent(QMouseEvent* const event)
{
  const auto rel_pos = m_transform.inverted().map(static_cast<QPointF>(event->pos()));
  const auto radius = std::sqrt(QPointF::dotProduct(rel_pos, rel_pos));

  const auto distance = [radius](const auto* const item) { return std::abs(item->radius() - radius); };
  const auto it = std::ranges::min_element(m_moveable_items, {}, distance);
  static constexpr auto eps = 0.2;
  if (distance(*it) < eps) {
    m_current_item = *it;
    m_current_item->set_active(true);
    m_current_item->move_to(event->pos());
  }
  if (const auto d = AMPMItem::pos - rel_pos; std::sqrt(QPointF::dotProduct(d, d)) < AMPMItem::size) {
    m_ampm_item->toggle();
    const std::lock_guard _(m_update_canvas_mutex);
    set_time(current_time());
  }
  update();
}

void TimeEdit::mouseReleaseEvent(QMouseEvent* const event)
{
  if (m_current_item) {
    m_current_item->set_active(false);
    m_current_item->quantify();
    set_time(current_time());
    update();
  }
  m_current_item = nullptr;
}

void TimeEdit::mouseMoveEvent(QMouseEvent* const event)
{
  if (m_current_item != nullptr) {
    m_current_item->move_to(event->pos());
    const std::lock_guard _(m_update_canvas_mutex);
    set_time(current_time());
    update();
  }
}

void TimeEdit::resizeEvent(QResizeEvent* const event)
{
  m_transform = {};
  m_transform.translate(static_cast<double>(event->size().width()) / 2.0,
                        static_cast<double>(event->size().height()) / 2.0);
  const auto scale = static_cast<double>(std::min(event->size().width(), event->size().height())) / 2.0;
  m_transform.scale(scale, scale);
  update();
}
