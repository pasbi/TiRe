#include "period.h"
#include "days.h"
#include "interval.h"

namespace
{

constexpr auto number_of_days_in_a_week = 7;
enum class Rim { Begin, End };

[[nodiscard]] QDate calculate_period(const QDate& date, const Period::Type type, const Rim rim)
{
  switch (type) {
    using enum Period::Type;
  case Day:
    return date;
  case Week:
    if (const auto begin = date.addDays(::monday - date.dayOfWeek()); rim == Rim::Begin) {
      return begin;
    } else {
      return begin.addDays(number_of_days_in_a_week - 1);
    }
  case Month:
    if (const auto begin = QDate{date.year(), date.month(), 1}; rim == Rim::Begin) {
      return begin;
    } else {
      return begin.addDays(date.daysInMonth());
    }
  case Year:
    if (const auto begin = QDate{date.year(), 1, 1}; rim == Rim::Begin) {
      return begin;
    } else {
      return begin.addDays(date.daysInYear());
    }
  }
  return {};
}

}  // namespace

Period::Period(const QDate& date, const Type type)
  : m_begin(::calculate_period(date, type, Rim::Begin)), m_end(::calculate_period(date, type, Rim::End)), m_type(type)
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

int Period::minutes_overlap(const Interval& interval) const noexcept
{
  const auto left = std::max(m_begin.startOfDay(), interval.begin());
  const auto right = std::min(m_end.endOfDay(), interval.end());
  static constexpr auto msecs_per_minute = 60LL * 1000LL;
  return static_cast<int>(left.msecsTo(right) / msecs_per_minute);
}
