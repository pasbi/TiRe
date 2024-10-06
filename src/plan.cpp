#include "plan.h"
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
  return 8h;
}

std::chrono::minutes Plan::planned_working_time(const Period& period) const noexcept
{
  using std::chrono_literals::operator""min;
  auto duration = 0min;
  // TODO clamping at the current date makes the `Planned` label for future periods display wrong values.
  // Planned should show the planned time until today (e.g., 16h for this week on a Tuesday), analogously, Overtime
  // should display the overtime until today (not -8h on a Thursday if you've worked for 8h each Mon-Thu but you're
  // missing Friday because that'd be only tomorrow).
  // So I think the way it is implemented right now is what we want.
  // We should, however, not display such "wrong" values, e.g., by hiding them in future periods.
  if (period.end() < m_start) {
    return duration;
  }
  const auto actual_end = std::clamp(period.end(), m_start, Application::current_date_time().date());

  const auto day_count = period.begin().daysTo(actual_end) + 1;
  for (qint64 day = 0; day < day_count; ++day) {
    duration += planned_working_time(period.begin().addDays(day));
  }
  return duration;
}

const std::chrono::minutes& Plan::overtime_offset() const noexcept
{
  return m_overtime_offset;
}

const QDate& Plan::start() const noexcept
{
  return m_start;
}
