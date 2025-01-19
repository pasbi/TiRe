#include "plan.h"

#include "enum.h"
#include "json.h"
#include "period.h"
#include "periodedit.h"

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

template<> struct nlohmann::adl_serializer<std::unique_ptr<Plan::Entry>>
{
  static void to_json(json& json_value, const std::unique_ptr<Plan::Entry>& ptr)
  {
    if (ptr == nullptr) {
      json_value = nullptr;
    } else {
      json_value = *ptr;
    }
  }

  static void from_json(const json& json_value, std::unique_ptr<Plan::Entry>& ptr)
  {
    if (json_value.is_null()) {
      ptr = {};
    } else {
      ptr = std::make_unique<Plan::Entry>(static_cast<Plan::Entry>(json_value));
    }
  }
};

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
  const auto it = std::ranges::find_if(m_periods, [&date](const auto& entry) { return entry->period.contains(date); });
  if (it == m_periods.end()) {
    return Kind::Normal;
  }
  return (*it)->kind;
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

  const auto& [period, kind] = *m_periods.at(index.row());
  switch (index.column()) {
  case period_column:
    return period.label();
  case kind_column:
    return QString::fromStdString(fmt::format("{}", kind));
  default:
    Q_UNREACHABLE();
  }
}

QVariant Plan::headerData(const int section, const Qt::Orientation orientation, const int role) const
{
  if (orientation == Qt::Vertical || role != Qt::DisplayRole) {
    return {};
  }
  switch (section) {
  case period_column:
    return tr("Period");
  case kind_column:
    return tr("Kind");
  default:
    Q_UNREACHABLE();
  }
}

Qt::ItemFlags Plan::flags(const QModelIndex& index) const
{
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

void Plan::add(std::unique_ptr<Entry> entry)
{
  const auto row = static_cast<int>(m_periods.size());
  beginInsertRows({}, row, row);
  m_periods.emplace_back(std::move(entry));
  endInsertRows();
  Q_EMIT plan_changed();
}

std::unique_ptr<Plan::Entry> Plan::extract(const Entry& entry)
{
  if (const auto it =
          std::ranges::find_if(m_periods, [&entry](const auto& candidate) { return candidate.get() == &entry; });
      it != m_periods.end())
  {
    const auto row = std::distance(m_periods.begin(), it);
    beginRemoveRows({}, row, row);
    auto ret = std::move(*it);
    m_periods.erase(it);
    endRemoveRows();
    Q_EMIT plan_changed();
    return ret;
  }
  return {};
}

const Plan::Entry& Plan::entry(const int row) const noexcept
{
  return *m_periods.at(row);
}

void Plan::data_changed(const int row, const int column)
{
  const auto index = this->index(row, column);
  Q_EMIT dataChanged(index, index);
  Q_EMIT plan_changed();
}

std::chrono::minutes Plan::count(const Period& period, const std::map<Kind, double>& factors) const
{
  using std::chrono_literals::operator""min;
  auto sum = 0min;
  for (const auto& entry : m_periods) {
    const auto it = factors.find(entry->kind);
    if (it == factors.end()) {
      continue;
    }
    const auto intersected_period = entry->period.overlap(period);
    if (!intersected_period.has_value()) {
      continue;
    }

    sum +=
        std::chrono::duration_cast<std::chrono::minutes>(it->second * planned_normal_working_time(*intersected_period));
  }
  return sum;
}

std::chrono::minutes Plan::planned_normal_working_time(const Period& period) const noexcept
{
  using std::chrono_literals::operator""min;
  auto result = 0min;
  for (const auto date : period.dates()) {
    result += planned_normal_working_time(date);
  }
  return result;
}

void Plan::set_data(const int row, const Kind kind)
{
  m_periods.at(row)->kind = kind;
  data_changed(row, period_column);
}

void Plan::set_data(const int row, const Period& period)
{
  m_periods.at(row)->period = period;
  data_changed(row, kind_column);
}

std::chrono::minutes Plan::sick_time(const Period& period) const
{
  return count(period, {
                           {Kind::Holiday, 0.0},
                           {Kind::Normal, 0.0},
                           {Kind::Sick, 1.0},
                           {Kind::Vacation, 0.0},
                           {Kind::HalfVacation, 0.0},
                           {Kind::HalfHoliday, 0.0},
                           {Kind::HalfVacationHalfHoliday, 0.0},
                       });
}

std::chrono::minutes Plan::holiday_time(const Period& period) const
{
  return count(period, {
                           {Kind::Holiday, 1.0},
                           {Kind::Normal, 0.0},
                           {Kind::Sick, 0.0},
                           {Kind::Vacation, 0.0},
                           {Kind::HalfVacation, 0.0},
                           {Kind::HalfHoliday, 0.5},
                           {Kind::HalfVacationHalfHoliday, 0.5},
                       });
}

std::chrono::minutes Plan::vacation_time(const Period& period) const
{
  return count(period, {
                           {Kind::Holiday, 0.0},
                           {Kind::Normal, 0.0},
                           {Kind::Sick, 0.0},
                           {Kind::Vacation, 1.0},
                           {Kind::HalfVacation, 0.5},
                           {Kind::HalfHoliday, 0.0},
                           {Kind::HalfVacationHalfHoliday, 0.5},
                       });
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
  value.kind = j.at(kind_key);
}

void to_json(nlohmann::json& j, const Plan::Kind& value)
{
  j = fmt::format("{}", value);
}

void from_json(const nlohmann::json& j, Plan::Kind& value)
{
  value = ::enum_from_string<Plan::Kind, 7>(j);
}
