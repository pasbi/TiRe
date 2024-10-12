#include "plan.h"

#include "enum.h"
#include "exceptions.h"
#include "fmt.h"
#include "period.h"
#include <QDate>
#include <nlohmann/json.hpp>

namespace
{
constexpr auto start_key = "start";
constexpr auto overtime_offset_key = "overtime_offset";
constexpr auto periods_key = "periods";
constexpr auto period_key = "period";
constexpr auto kind_key = "kind";
}  // namespace

Plan::Plan(const nlohmann::json& data) : m_start(data.at(start_key)), m_overtime_offset(data.at(overtime_offset_key))
{
  if (data.contains(periods_key)) {
    m_periods = data.at(periods_key);
  }
}

Plan::Plan()
{
}

nlohmann::json Plan::to_json() const noexcept
{
  return {
      {start_key, m_start},
      {overtime_offset_key, m_overtime_offset},
      {periods_key, m_periods},
  };
}

std::chrono::minutes Plan::planned_working_time(const QDate& date, const std::chrono::minutes actual_working_time) const
{
  const auto planned_normal_working_time = this->planned_normal_working_time(date);
  using enum Kind;
  using std::chrono_literals::operator""min;
  switch (find_kind(date)) {
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

Plan::Kind Plan::find_kind(const QDate& date) const
{
  const auto it = std::ranges::find_if(m_periods, [&date](const auto& entry) { return entry.period.contains(date); });
  if (it == m_periods.end()) {
    return Kind::Normal;
  }
  return it->kind;
}

int Plan::columnCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : 2;
}

int Plan::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid()) {
    return 0;
  }
  return static_cast<int>(m_periods.size());
}

QVariant Plan::data(const QModelIndex& index, const int role) const
{
  if (role != Qt::DisplayRole) {
    return {};
  }

  const auto& [period, kind] = m_periods.at(index.row());
  switch (index.column()) {
  case date_column:
    return period.label();
  case kind_column:
    return QString::fromStdString(fmt::format("{}", kind));
  }
  Q_UNREACHABLE();
}

std::chrono::minutes FullTimePlan::planned_normal_working_time(const QDate& date) const noexcept
{
  using std::chrono_literals::operator""min;
  using std::chrono_literals::operator""h;
  const auto day = date.dayOfWeek();
  return day == Qt::Saturday || day == Qt::Sunday ? 0min : 8h;
}

void to_json(nlohmann::json& j, const Plan::Entry& value)
{
  j = {
      {period_key, value.period},
      {kind_key, value.kind},
  };
}

void from_json(const nlohmann::json& j, Plan::Entry& value)
{
  value.period = j.at(period_key);
  value.period = j.at(kind_key);
}

void to_json(nlohmann::json& j, const Plan::Kind& value)
{
  j = fmt::format("{}", value);
}

void from_json(const nlohmann::json& j, Plan::Kind& value)
{
  value = ::enum_from_string<Plan::Kind, 7>(j);
}
