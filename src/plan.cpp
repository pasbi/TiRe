#include "plan.h"

#include "db/abstracttimesheetrepository.h"
#include "exceptions.h"
#include "intervalmodel.h"
#include "period.h"
#include "periodedit.h"

#include <QDate>
#include <algorithm>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>

namespace
{

/** @brief Whether a day's accumulated leave can be accounted for, and if not, why not. */
[[nodiscard]] Plan::Conflict conflict_of(const Plan::DayShares& shares) noexcept
{
  using enum Plan::Conflict;
  if (shares[Plan::Kind::Sick] != Plan::Share::none() && shares[Plan::Kind::ChildSick] != Plan::Share::none()) {
    // Both waivers cover only the hours actually missed, so a day carrying both could not say
    // which allowance the missed hours came out of -- and one of the two is capped by law.
    return MixedSickness;
  }
  if (shares.budget() + shares.waiver() > Plan::Share::whole()) {
    return Overbooked;
  }
  return None;
}

}  // namespace

Plan::Share Plan::DayShares::operator[](const Kind kind) const noexcept
{
  return m_shares.at(static_cast<std::size_t>(kind));
}

void Plan::DayShares::add(const Kind kind, const Share share) noexcept
{
  auto& own = m_shares.at(static_cast<std::size_t>(kind));
  own = own + share;
}

Plan::Share Plan::DayShares::budget() const noexcept
{
  return (*this)[Kind::Holiday] + (*this)[Kind::Vacation];
}

Plan::Share Plan::DayShares::waiver() const noexcept
{
  return (*this)[Kind::Sick] + (*this)[Kind::ChildSick];
}

bool Plan::DayShares::empty() const noexcept
{
  return budget() + waiver() == Share::none();
}

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
  for (const auto& entry : m_periods) {
    // Checked against the plan without the entry itself, which is exactly what check() does with
    // a `replaced` argument -- every entry gets to see all the others.
    if (check(*entry, entry.get()) != Conflict::None) {
      throw DatabaseError("The stored plan books more leave on a day than it can hold.");
    }
  }
}

std::chrono::minutes Plan::planned_working_time(const QDate& date, const DayShares& shares,
                                                const IntervalModel& interval_model) const noexcept
{
  using std::chrono_literals::operator""min;
  const auto normal = planned_normal_working_time(date);
  // Budget leave is spent whatever happens, so it lowers the target outright.
  const auto after_budget = std::max(0min, normal - shares.budget().of(normal));
  // A waiver excuses at most its share, and only as far as the time was actually missed: the
  // floor is what is still owed once the waiver is exhausted -- zero for a whole sick day, so
  // any amount of work is covered -- and the ceiling is why working through a sick day earns
  // overtime like any other day. The min() also keeps the clamp well-formed if stored data ever
  // claims more than the day can hold.
  const auto waived = std::min(shares.waiver().of(normal), after_budget);
  return std::clamp(interval_model.minutes(date), after_budget - waived, after_budget);
}

void Plan::sort() noexcept
{
  // Stable, because entries may share a beginning and their relative order is all that tells two
  // such rows apart in the view.
  std::ranges::stable_sort(m_periods, std::less<>{}, [](const auto& entry) { return entry->period.begin(); });
}

Plan::DayShares Plan::shares_on(const QDate& date) const
{
  return shares_on(date, nullptr);
}

Plan::DayShares Plan::shares_on(const QDate& date, const Entry* const excluded) const
{
  DayShares shares;
  for (const auto& entry : m_periods) {
    if (entry.get() != excluded && entry->period.contains(date)) {
      shares.add(entry->kind, entry->share);
    }
  }
  return shares;
}

std::vector<Plan::DayShares> Plan::shares_in(const Period& period) const
{
  if (!period.begin().isValid() || !period.end().isValid()) {
    return {};
  }

  std::vector<DayShares> shares(static_cast<std::size_t>(period.days()));
  assert(is_sorted());
  for (const auto& entry : m_periods) {
    if (entry->period.begin() > period.end()) {
      // we are past the interesting periods
      break;
    }
    const auto overlap = entry->period.overlap(period);
    if (!overlap.has_value()) {
      continue;
    }
    const auto first = static_cast<std::size_t>(period.begin().daysTo(overlap->begin()));
    for (const auto i : std::views::iota(first, first + static_cast<std::size_t>(overlap->days()))) {
      shares.at(i).add(entry->kind, entry->share);
    }
  }
  return shares;
}

std::chrono::minutes Plan::planned_working_time(const Period& period, const IntervalModel& interval_model) const
{
  const auto shares = shares_in(period);
  using std::chrono_literals::operator""min;
  auto sum = 0min;
  for (const auto i : std::views::iota(0uz, shares.size())) {
    // TODO planned_working_time calls planned_normal_working_time which is virtual.
    // The virtual lookup doesn't need to be done each time, the runtime type is the same in each iteration, only
    // the function argument changes.
    sum += planned_working_time(period.begin().addDays(static_cast<qint64>(i)), shares.at(i), interval_model);
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

Period Plan::default_period() const noexcept
{
  // Walks to the first day that carries no leave at all, so that the whole-day entry the "add
  // plan entry" action builds around this period is one add() will actually accept. The last
  // entry bounds the walk, so it terminates.
  auto date = Application::current_date_time().date();
  while (!shares_on(date).empty()) {
    date = date.addDays(1);
  }
  return Period{date, Period::Type::Day};
}

int Plan::columnCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : 3;
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
  case share_column:
    return QString::fromStdString(fmt::format("{}", entry.share));
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
  case share_column:
    return tr("Share");
  default:
    Q_UNREACHABLE();
  }
}

Qt::ItemFlags Plan::flags(const QModelIndex& index) const
{
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

bool Plan::add(std::unique_ptr<Entry> entry)
{
  if (const auto conflict = check_add(*entry); conflict != Conflict::None) {
    // Caution: returning false here destroys `entry`. An AddCommand that hit this would be left
    // holding a dangling reference, so callers must ensure the booking fits first -- see
    // check_add() and default_period(). Kept loud rather than silent for that reason.
    spdlog::error("Refusing to add {} of {} on {}: it does not fit the leave already booked. The entry is dropped.",
                  entry->share, entry->kind, entry->period);
    return false;
  }

  static constexpr auto begin_of = [](const auto& candidate) { return candidate->period.begin(); };
  const auto insert_pos = std::ranges::upper_bound(m_periods, entry->period.begin(), std::less<>{}, begin_of);
  const auto row = static_cast<int>(std::distance(m_periods.begin(), insert_pos));
  beginInsertRows({}, row, row);
  auto& ref = **m_periods.insert(insert_pos, std::move(entry));
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
    const auto row = static_cast<int>(std::distance(m_periods.begin(), it));
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
  if (check_kind(entry, kind) != Conflict::None) {
    throw RuntimeError("Failed to change kind because the leave would not fit.");
  }
  swap(entry.kind, kind);
  m_repository.update(entry);
  data_changed(row_of(entry), kind_column);
  return kind;
}

Plan::Share Plan::swap_share(const Entry& entry_ref, Share share)
{
  using std::swap;
  auto& entry = find_entry(entry_ref);
  if (check_share(entry, share) != Conflict::None) {
    throw RuntimeError("Failed to change share because the leave would not fit.");
  }
  swap(entry.share, share);
  m_repository.update(entry);
  data_changed(row_of(entry), share_column);
  return share;
}

Period Plan::swap_period(const Entry& entry_ref, Period period)
{
  using std::swap;
  auto& entry = find_entry(entry_ref);
  if (check_period(entry, period) != Conflict::None) {
    throw RuntimeError("Failed to change period because the leave would not fit.");
  }
  swap(entry.period, period);
  // A new period generally belongs at a different row, so this is a reordering, not a cell edit.
  // Announcing it as a reset keeps the view and any persistent indices honest -- emitting only
  // dataChanged for one cell would leave every other row displaying the wrong entry.
  beginResetModel();
  sort();
  endResetModel();
  m_repository.update(entry);
  Q_EMIT plan_changed();
  return period;
}

Plan::Conflict Plan::check(const Entry& candidate, const Entry* const replaced) const
{
  if (candidate.share <= Share::none() || candidate.share > Share::whole()) {
    return Conflict::ImpossibleShare;
  }
  for (const auto& date : candidate.period.dates()) {
    auto shares = shares_on(date, replaced);
    shares.add(candidate.kind, candidate.share);
    if (const auto conflict = ::conflict_of(shares); conflict != Conflict::None) {
      return conflict;
    }
  }
  return Conflict::None;
}

Plan::Conflict Plan::check_add(const Entry& candidate) const
{
  return check(candidate, nullptr);
}

Plan::Conflict Plan::check_kind(const Entry& entry, const Kind kind) const
{
  return check({entry.period, kind, entry.share, entry.id}, &entry);
}

Plan::Conflict Plan::check_share(const Entry& entry, const Share share) const
{
  return check({entry.period, entry.kind, share, entry.id}, &entry);
}

Plan::Conflict Plan::check_period(const Entry& entry, const Period& period) const
{
  return check({period, entry.kind, entry.share, entry.id}, &entry);
}

std::chrono::minutes Plan::leave_time(const Kind kind, const Period& period, const IntervalModel& interval_model) const
{
  const auto shares = shares_in(period);
  using std::chrono_literals::operator""min;
  auto sum = 0min;
  // Driven by the shares rather than by period.days(), so that an invalid period, for which
  // shares_in yields nothing, counts nothing instead of running off the end.
  for (const auto i : std::views::iota(0uz, shares.size())) {
    const auto& day = shares.at(i);
    if (day[kind] == Share::none()) {
      continue;
    }
    const auto date = period.begin().addDays(static_cast<qint64>(i));
    const auto normal = planned_normal_working_time(date);
    if (is_budget(kind)) {
      sum += day[kind].of(normal);
    } else {
      // The time the waiver had to cover: the target the day would have had without it, minus
      // the target it ended up with. A day the plan expects no work on waives nothing, so
      // weekends drop out here without a special case. Only one sickness may share a day (see
      // Conflict::MixedSickness), so all of it belongs to this kind.
      sum += std::max(0min, normal - day.budget().of(normal)) - planned_working_time(date, day, interval_model);
    }
  }
  return sum;
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
  static constexpr auto begin_of = [](const auto& entry) { return entry->period.begin(); };
  return std::ranges::is_sorted(m_periods, std::less<>{}, begin_of);
}
