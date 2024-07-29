#include "plan.h"
#include "json.h"
#include "period.h"
#include <QDate>
#include <nlohmann/json.hpp>

namespace
{
constexpr auto start_key = "start";
constexpr auto overtime_offset_key = "overtime_offset";
constexpr auto planned_working_time_key = "planed_working_time";
}  // namespace

Plan::Plan(const nlohmann::json& data)
  : m_start(data.at(start_key))
  , m_overtime_offset(data.at(overtime_offset_key))
  , m_planned_working_time(data.at(planned_working_time_key))
{
}

nlohmann::json Plan::to_json() const noexcept
{
  return {
      {start_key, m_start},
      {overtime_offset_key, m_overtime_offset},
      {planned_working_time_key, m_planned_working_time},
  };
}

std::chrono::minutes Plan::planned_working_time(const QDate& date) const noexcept
{
  if (const auto it = m_planned_working_time.find(date); it != m_planned_working_time.end()) {
    return it->second;
  }
  if (const auto day = date.dayOfWeek(); day == Qt::Saturday || day == Qt::Sunday) {
    using std::chrono_literals::operator""min;
    return 0min;
  }
  using std::chrono_literals::operator""h;
  return std::chrono::duration_cast<std::chrono::minutes>(8h);
}

std::chrono::minutes Plan::planned_working_time(const QDate& begin, const QDate& end) const noexcept
{
  using std::chrono_literals::operator""min;
  auto duration = 0min;
  const auto actual_begin = std::clamp(begin.isValid() ? begin : m_start, m_start, QDate::currentDate());
  if (end < m_start) {
    return duration;
  }
  const auto actual_end = std::clamp(end, m_start, QDate::currentDate());

  const auto day_count = actual_begin.daysTo(actual_end) + 1;
  for (qint64 day = 0; day < day_count; ++day) {
    duration += planned_working_time(actual_begin.addDays(day));
  }
  return duration;
}

const std::chrono::minutes& Plan::overtime_offset() const noexcept
{
  return m_overtime_offset;
}