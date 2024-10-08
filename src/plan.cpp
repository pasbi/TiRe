#include "plan.h"
#include "fmt.h"
#include "period.h"
#include <QDate>
#include <nlohmann/json.hpp>

namespace
{
constexpr auto start_key = "start";
constexpr auto overtime_offset_key = "overtime_offset";
constexpr auto planned_working_time_key = "planed_working_time";
}  // namespace

Plan::Plan(const nlohmann::json& data) : m_start(data.at(start_key)), m_overtime_offset(data.at(overtime_offset_key))
{
  m_kinds.try_emplace(m_start, Kind::Normal);  // TODO
}

Plan::Plan()
{
  m_kinds.try_emplace(m_start, Kind::Normal);  // TODO
}

nlohmann::json Plan::to_json() const noexcept
{
  return {
      {start_key, m_start},
      {overtime_offset_key, m_overtime_offset},
  };
}

std::chrono::minutes Plan::planned_working_time(const QDate& date, const std::chrono::minutes actual_working_time) const
{
  const auto planned_normal_working_time = this->planned_normal_working_time(date);
  using enum Kind;
  using std::chrono_literals::operator""min;
  switch (find_kind(date)->second) {
  case Normal:
    return planned_normal_working_time;
  case Holiday:
  case Vacation:
  case HalfVacationHalfHoliday:
    return 0min;
  case Sick:
    return std::min(actual_working_time, planned_normal_working_time);
  case HalfHoliday:
  case HalfVacation:
    return planned_normal_working_time / 2;
  }
  Q_UNREACHABLE();
  return 0min;
}

const std::chrono::minutes& Plan::overtime_offset() const noexcept
{
  return m_overtime_offset;
}

const QDate& Plan::start() const noexcept
{
  return m_start;
}

std::map<QDate, Plan::Kind>::const_iterator Plan::find_kind(const QDate& date) const
{
  if (m_kinds.empty()) {
    throw std::runtime_error("m_kinds is empty.");
  }
  const auto lower_bound = m_kinds.lower_bound(date);
  if (lower_bound->first == date) {
    return lower_bound;
  }
  if (lower_bound == m_kinds.begin()) {
    throw std::runtime_error(fmt::format("Date {} predates all records (first: {})", date, m_kinds.begin()->first));
  }
  return std::prev(lower_bound);
}

std::chrono::minutes FullTimePlan::planned_normal_working_time(const QDate& date) const noexcept
{
  using std::chrono_literals::operator""min;
  using std::chrono_literals::operator""h;
  const auto day = date.dayOfWeek();
  return day == Qt::Saturday || day == Qt::Sunday ? 0min : 8h;
}
