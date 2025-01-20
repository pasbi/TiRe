#include "plan.h"

#include "enum.h"
#include "intervalmodel.h"
#include "json.h"
#include "period.h"
#include "periodedit.h"

#include <QDate>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace
{
constexpr auto start_key = "start";
constexpr auto overtime_offset_key = "overtime_offset";
constexpr auto periods_key = "periods";
constexpr auto period_key = "period";
constexpr auto kind_key = "kind";

struct SickLeaveFactors
{
  using enum Plan::Kind;
  [[nodiscard]] static constexpr double factor(const Plan::Kind kind) noexcept
  {
    switch (kind) {
    case Sick:
      return 1.0;
    default:
      return 0.0;
    }
  }
};

struct VacationLeaveFactors
{
  using enum Plan::Kind;
  [[nodiscard]] static constexpr double factor(const Plan::Kind kind) noexcept
  {
    switch (kind) {
    case Holiday:
      return 1.0;
    case Vacation:
      return 1.0;
    case HalfVacation:
      return 0.5;
    case HalfVacationHalfHoliday:
      return 0.5;
    default:
      return 0.0;
    }
  }
};

struct HolidayLeaveFactors
{
  using enum Plan::Kind;
  [[nodiscard]] static constexpr double factor(const Plan::Kind kind) noexcept
  {
    switch (kind) {
    case Holiday:
      return 1.0;
    case HalfHoliday:
      return 0.5;
    case HalfVacationHalfHoliday:
      return 0.5;
    default:
      return 0.0;
    }
  }
};

[[nodiscard]] bool is_sorted(const std::vector<std::unique_ptr<Plan::Entry>>& vector)
{
  if (vector.empty()) {
    return true;
  }
  return std::ranges::all_of(std::views::iota(static_cast<std::size_t>(1), vector.size()), [&vector](const auto i) {
    return vector.at(i - 1)->period.end() <= vector.at(i)->period.begin();
  });
}

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
  std::ranges::sort(m_periods, std::less<>{}, [](const auto& entry) { return entry->period.begin(); });
  if (!::is_sorted(m_periods)) {
    throw RuntimeError("Failed to sort periods in plan: overlapping periods cannot be sorted.");
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

std::chrono::minutes Plan::planned_working_time(const QDate& date, const Kind kind,
                                                const IntervalModel& interval_model) const noexcept
{
  using enum Kind;
  switch (kind) {
  case Normal:
    return planned_normal_working_time(date);
  case Holiday:
  case Vacation:
  case HalfVacationHalfHoliday:
    using std::chrono_literals::operator""min;
    return 0min;
  case Sick:
    return std::min(interval_model.minutes(date), planned_normal_working_time(date));
  case HalfHoliday:
  case HalfVacation:
    return planned_normal_working_time(date) / 2;
  }
  Q_UNREACHABLE();
}

std::vector<Plan::Kind> Plan::kinds_in(const Period& period) const
{
  if (!period.begin().isValid() || !period.end().isValid()) {
    return {};
  }

  std::vector<Kind> kinds;
  kinds.reserve(period.days());
  Period active_period = period;
  assert(::is_sorted(m_periods));
  for (const auto& p : m_periods) {
    if (p->period.begin() > period.end()) {
      // we are past the interesting periods
      break;
    }
    if (p->period.end() < period.begin()) {
      // we haven't yet reached the interesting periods
      continue;
    }
    kinds.insert(kinds.end(), active_period.begin().daysTo(p->period.begin()), Kind::Normal);
    const auto overlap = p->period.overlap(active_period);
    assert(overlap.has_value());
    kinds.insert(kinds.end(), overlap->days(), p->kind);
    active_period = Period{p->period.end().addDays(1), active_period.end()};
  }
  kinds.insert(kinds.end(), active_period.days(), Kind::Normal);
  return kinds;
}

std::chrono::minutes Plan::planned_working_time(const Period& period, const IntervalModel& interval_model) const
{
  const std::vector<Kind> kinds = kinds_in(period);
  using std::chrono_literals::operator""min;
  auto sum = 0min;
  for (const auto i : std::views::iota(0, period.days())) {
    // TODO planned_working_time calls planned_normal_working_time which is virtual.
    // The virtual lookup doesn't need to be done each time, the runtime type is the same in each iteration, only
    // the function argument changes.
    sum += planned_working_time(period.begin().addDays(i), kinds.at(i), interval_model);
  }
  return sum;
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

std::optional<std::vector<std::unique_ptr<Plan::Entry>>::const_iterator>
find_period_insert_pos(const std::vector<std::unique_ptr<Plan::Entry>>& periods, const Period& period) noexcept
{
  static constexpr auto projection = [](const auto& e) { return e->period.begin(); };
  const auto insert_pos = std::ranges::upper_bound(periods, period.begin(), std::less<>{}, projection);
  if (insert_pos != periods.end() && period.end() >= (*insert_pos)->period.begin()) {
    // there is a subsequent period and its beginning is before the candidate's end.
    return {};
  }
  if (insert_pos != periods.begin() && (*(insert_pos - 1))->period.end() >= period.begin()) {
    // there is a previous period and its end is after the candidate's begin.
    return {};
  }
  // If periods before or after the candidate exist, then they do not overlap with the candidate
  return insert_pos;
}

bool Plan::add(std::unique_ptr<Entry> entry)
{
  const auto insert_pos = find_period_insert_pos(m_periods, entry->period);
  if (!insert_pos.has_value()) {
    return false;
  }

  const auto row = std::distance(m_periods.cbegin(), *insert_pos);
  beginInsertRows({}, row, row);
  m_periods.insert(*insert_pos, std::move(entry));
  endInsertRows();
  Q_EMIT plan_changed();
  assert(::is_sorted(m_periods));
  return true;
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

template<typename LeaveFactors> std::chrono::minutes Plan::count(const Period& period) const
{
  using std::chrono_literals::operator""min;
  auto sum = 0min;
  for (const auto& entry : m_periods) {
    const auto factor = LeaveFactors::factor(entry->kind);
    const auto intersected_period = entry->period.overlap(period);
    if (!intersected_period.has_value()) {
      continue;
    }

    sum += std::chrono::duration_cast<std::chrono::minutes>(factor * planned_normal_working_time(*intersected_period));
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
  return count<SickLeaveFactors>(period);
}

std::chrono::minutes Plan::holiday_time(const Period& period) const
{
  return count<HolidayLeaveFactors>(period);
}

std::chrono::minutes Plan::vacation_time(const Period& period) const
{
  return count<VacationLeaveFactors>(period);
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
