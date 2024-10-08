#include "period.h"
#include "fmt.h"
#include "interval.h"
#include <chrono>
#include <fmt/chrono.h>
#include <spdlog/spdlog.h>

namespace
{

constexpr auto number_of_days_in_a_week = 7;
enum class Rim { Begin, End };

[[nodiscard]] auto type_label(const Period::Type type)
{
  switch (type) {
    using enum Period::Type;
  case Day:
    return QObject::tr("Day");
  case Week:
    return QObject::tr("Week");
  case Month:
    return QObject::tr("Month");
  case Year:
    return QObject::tr("Year");
  }
  return QString{};
}

[[nodiscard]] QDate calculate_period(const QDate& date, const Period::Type type, const Rim rim)
{
  switch (type) {
    using enum Period::Type;
  case Day:
    return date;
  case Week:
    if (const auto begin = date.addDays(Qt::Monday - date.dayOfWeek()); rim == Rim::Begin) {
      return begin;
    } else {
      return begin.addDays(number_of_days_in_a_week - 1);
    }
  case Month:
    if (const auto begin = QDate{date.year(), date.month(), 1}; rim == Rim::Begin) {
      return begin;
    } else {
      return begin.addDays(date.daysInMonth() - 1);
    }
  case Year:
    if (const auto begin = QDate{date.year(), 1, 1}; rim == Rim::Begin) {
      return begin;
    } else {
      return begin.addDays(date.daysInYear() - 1);
    }
  }
  return {};
}

}  // namespace

Period::Period(const QDate& date, const Type type)
  : m_begin(::calculate_period(date, type, Rim::Begin)), m_end(::calculate_period(date, type, Rim::End)), m_type(type)
{
}

Period::Period(const QDate& begin, const QDate& end) : m_begin(begin), m_end(end), m_type(Type::Custom)
{
}

const QDate& Period::begin() const noexcept
{
  return m_begin;
}

const QDate& Period::end() const noexcept
{
  return m_end;
}

Period::Type Period::type() const noexcept
{
  return m_type;
}

std::chrono::minutes Period::overlap(const Interval& interval) const noexcept
{
  const auto begin = std::max(m_begin.startOfDay(), interval.begin());
  const auto end = std::min(m_end.endOfDay(), interval.end());
  using std::chrono_literals::operator""ms;
  if (begin < end) {
    return std::chrono::duration_cast<std::chrono::minutes>(begin.msecsTo(end) * 1ms);
  }
  using std::chrono_literals::operator""min;
  return 0min;
}

QString Period::label() const
{
  if (!m_begin.isValid() || !m_end.isValid()) {
    return QObject::tr("-");
  }
  switch (m_type) {
    using enum Type;
  case Year:
    return QObject::tr("Year %1").arg(m_begin.year());
  case Month:
    return m_begin.toString("MMMM yyyy");
  case Week: {
    int year;
    const auto week_number = m_begin.weekNumber(&year);
    return QObject::tr("Week %1 in %2").arg(week_number).arg(year);
  }
  case Day:
    return m_begin.toString("dddd, dd.MM.yyyy");
  }
  return {};
}

bool Period::contains(const Period& period) const noexcept
{
  return std::max(m_begin, period.m_begin) <= std::min(m_end, period.m_end);
}

bool Period::contains(const QDate& date) const noexcept
{
  return m_begin <= date && date <= m_end;
}

std::weak_ordering operator<=>(const Period& a, const Period& b) noexcept
{
  return a.dates() <=> b.dates();
}

fmt::formatter<Period>::format_return_type fmt::formatter<Period>::format(const Period& p, fmt::format_context& ctx)
{
  return fmt::format_to(ctx.out(), "{}({}, {})", p.type(), p.begin(), p.end());
}

fmt::formatter<Period>::format_return_type fmt::formatter<Period::Type>::format(const Period::Type& t,
                                                                                fmt::format_context& ctx) const
{
  return fmt::format_to(ctx.out(), "{}", ::type_label(t));
}

int Period::days() const noexcept
{
  return static_cast<int>(m_begin.daysTo(m_end)) + 1;
}

QDate Period::clamp(const QDate& date) const noexcept
{
  if (!date.isValid()) {
    return date;
  }

  return std::clamp(date, begin(), end());
}

QDateTime Period::clamp(const QDateTime& date_time) const noexcept
{
  const auto date = clamp(date_time.date());
  return {date, date_time.time()};
}

Period Period::constrained(const QDate& latest_begin, const QDate& earliest_end) const
{
  if (m_type == Type::Custom) {
    return Period{std::min(m_begin, latest_begin), std::max(m_end, earliest_end)};
  }

  if (m_end < latest_begin || !m_end.isValid()) {
    return Period(latest_begin, m_type);
  }

  if (m_begin > earliest_end || !m_begin.isValid()) {
    return Period(earliest_end, m_type);
  }

  return *this;
}

std::pair<QDate, QDate> Period::dates() const noexcept
{
  return {m_begin, m_end};
}
