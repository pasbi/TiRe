#include "plan.h"

#include "db/abstracttimesheetrepository.h"
#include "exceptions.h"
#include "intervalmodel.h"
#include "period.h"
#include "periodedit.h"

#include <QDate>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>

namespace
{
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

}  // namespace

Plan::Plan() : Plan(null_repository())
{
}

Plan::Plan(AbstractTimeSheetRepository& repository) : m_repository(repository)
{
}

Plan::Plan(AbstractTimeSheetRepository& repository, const QDate& start, const std::chrono::minutes overtime_offset,
           std::vector<std::unique_ptr<Entry>> entries)
  : m_repository(repository), m_start(start), m_overtime_offset(overtime_offset), m_periods(std::move(entries))
{
  // Deliberately not going through add(): these entries are already stored, and add() would
  // write every one of them straight back.
  sort();
  if (!is_sorted()) {
    throw DatabaseError("The stored plan contains overlapping periods and cannot be sorted.");
  }
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

void Plan::sort() noexcept
{
  std::ranges::sort(m_periods, std::less<>{}, [](const auto& entry) { return entry->period.begin(); });
}

std::vector<Plan::Kind> Plan::kinds_in(const Period& period) const
{
  if (!period.begin().isValid() || !period.end().isValid()) {
    return {};
  }

  std::vector<Kind> kinds;
  kinds.reserve(period.days());
  Period active_period = period;
  assert(is_sorted());
  for (const auto& p : m_periods) {
    if (p->period.begin() > period.end()) {
      // we are past the interesting periods
      break;
    }
    if (p->period.end() < period.begin()) {
      // we haven't yet reached the interesting periods
      continue;
    }
    const auto n = std::max(static_cast<qint64>(0), active_period.begin().daysTo(p->period.begin()));
    kinds.insert(kinds.end(), n, Kind::Normal);
    const auto overlap = p->period.overlap(active_period);
    assert(overlap.has_value());
    kinds.insert(kinds.end(), overlap->days(), p->kind);
    active_period = Period{p->period.end().addDays(1), active_period.end()};
  }
  kinds.insert(kinds.end(), std::max(0, active_period.days()), Kind::Normal);
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

Period Plan::default_period() const noexcept
{
  const auto date = Application::current_date_time().date();
  const Period candidate{date, Period::Type::Day};
  // has_value(), not "== end()": find_period_insert_pos returns an optional, and comparing it to
  // end() asked for "insertable *and* last", so today was skipped whenever any later entry
  // existed.
  if (find_period_insert_pos(m_periods, candidate).has_value()) {
    return candidate;
  }
  return Period{m_periods.back()->period.end().addDays(1), Period::Type::Day};
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

  const auto& entry = *m_periods.at(index.row());
  switch (index.column()) {
  case period_column:
    return entry.period.label();
  case kind_column:
    return QString::fromStdString(fmt::format("{}", entry.kind));
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
    // Caution: returning false here destroys `entry`. An AddCommand that hit this would be left
    // holding a dangling reference, so callers must ensure the period is free first -- see
    // can_set_period() and default_period(). Kept loud rather than silent for that reason.
    spdlog::error("Refusing to add {} because it overlaps the existing periods {}. The entry is dropped.",
                  entry->period, m_periods | std::views::transform([](const auto& e) { return e->period; }));
    return false;
  }

  const auto row = std::distance(m_periods.cbegin(), *insert_pos);
  beginInsertRows({}, row, row);
  auto& ref = **m_periods.insert(*insert_pos, std::move(entry));
  endInsertRows();
  m_repository.insert(ref);
  Q_EMIT plan_changed();
  assert(is_sorted());
  return true;
}

std::unique_ptr<Plan::Entry> Plan::extract(const Entry& entry)
{
  if (const auto it =
          std::ranges::find_if(m_periods, [&entry](const auto& candidate) { return candidate.get() == &entry; });
      it != m_periods.end())
  {
    // Delete the row before touching the container, so a failure leaves the plan untouched rather
    // than destroying the entry during unwinding and dangling the calling command's reference.
    m_repository.remove(entry);
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

QDate Plan::swap_start(QDate start)
{
  using std::swap;
  swap(m_start, start);
  m_repository.update_plan_setting(*this);
  Q_EMIT plan_changed();
  return start;
}

std::chrono::minutes Plan::swap_overtime_offset(std::chrono::minutes overtime_offset)
{
  using std::swap;
  swap(m_overtime_offset, overtime_offset);
  m_repository.update_plan_setting(*this);
  Q_EMIT plan_changed();
  return overtime_offset;
}

Plan::Entry& Plan::find_entry(const Entry& entry)
{
  const auto it = std::ranges::find(m_periods, &entry, [](const auto& candidate) { return candidate.get(); });
  if (it == m_periods.end()) {
    throw RuntimeError("The entry does not belong to this plan.");
  }
  return **it;
}

int Plan::row_of(const Entry& entry) const
{
  const auto it = std::ranges::find(m_periods, &entry, [](const auto& candidate) { return candidate.get(); });
  return static_cast<int>(std::distance(m_periods.begin(), it));
}

Plan::Kind Plan::swap_kind(const Entry& entry_ref, Kind kind)
{
  using std::swap;
  auto& entry = find_entry(entry_ref);
  swap(entry.kind, kind);
  m_repository.update(entry);
  data_changed(row_of(entry), kind_column);
  return kind;
}

Period Plan::swap_period(const Entry& entry_ref, Period period)
{
  auto& entry = find_entry(entry_ref);
  swap(entry.period, period);
  // A new period generally belongs at a different row, so this is a reordering, not a cell edit.
  // Announcing it as a reset keeps the view and any persistent indices honest -- emitting only
  // dataChanged for one cell would leave every other row displaying the wrong entry.
  beginResetModel();
  sort();
  const auto sorted = is_sorted();
  if (!sorted) {
    // Put it back before anyone observes the broken ordering, and persist nothing.
    swap(entry.period, period);
    sort();
  }
  endResetModel();
  if (!sorted) {
    throw RuntimeError("Failed to change period because it would overlap.");
  }
  m_repository.update(entry);
  Q_EMIT plan_changed();
  return period;
}

bool Plan::can_set_period(const Entry& entry, const Period& period) const
{
  // The entry being moved is excluded, because it may of course overlap its own current period.
  // Period::overlap treats touching periods (one's end equal to the next one's begin) as
  // overlapping, which is the same rule is_sorted() enforces, so this agrees with swap_period().
  const auto is_other = [&entry](const auto& candidate) { return candidate.get() != &entry; };
  const auto overlaps = [&period](const auto& candidate) { return candidate->period.overlap(period).has_value(); };
  return std::ranges::none_of(m_periods | std::views::filter(is_other), overlaps);
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

[[nodiscard]] bool Plan::is_sorted() const noexcept
{
  return m_periods.empty()
         || std::ranges::all_of(std::views::iota(static_cast<std::size_t>(1), m_periods.size()), [this](const auto i) {
              return m_periods.at(i - 1)->period.end() < m_periods.at(i)->period.begin();
            });
}
