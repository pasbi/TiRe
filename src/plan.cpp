#include "plan.h"
#include "fmt.h"
#include "period.h"
#include <QDate>
#include <nlohmann/json.hpp>

namespace
{
constexpr auto start_key = "start";
constexpr auto overtime_offset_key = "overtime_offset";
constexpr auto kinds_key = "kinds";
}  // namespace

Plan::Plan(const nlohmann::json& data)
  : m_start(data.at(start_key))
  , m_overtime_offset(data.at(overtime_offset_key))
  , m_kinds(data.contains(kinds_key) ? data.at(kinds_key) : nlohmann::json::array({{m_start, Kind::Normal}}))
{
}

Plan::Plan()
{
  m_kinds.try_emplace(m_start, Kind::Normal);
}

nlohmann::json Plan::to_json() const noexcept
{
  return {
      {start_key, m_start},
      {overtime_offset_key, m_overtime_offset},
      {kinds_key, m_kinds},
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
}

const std::chrono::minutes& Plan::overtime_offset() const noexcept
{
  return m_overtime_offset;
}

const QDate& Plan::start() const noexcept
{
  return m_start;
}

Plan::KindMap::const_iterator Plan::find_kind(const QDate& date) const
{
  if (m_kinds.empty()) {
    throw std::runtime_error("m_kinds is empty.");
  }
  const auto lower_bound = m_kinds.lower_bound(date);
  if (lower_bound->first == date) {
    return lower_bound;
  }
  if (lower_bound == m_kinds.begin()) {
    m_kinds.end();
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
