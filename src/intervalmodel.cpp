#include "intervalmodel.h"
#include "colorutil.h"
#include "period.h"
#include <QColor>
#include <complex>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace
{

template<typename Intervals> [[nodiscard]] auto find(Intervals&& intervals, const Interval& interval)
{
  const auto it = std::ranges::find(intervals, &interval, &std::unique_ptr<Interval>::get);
  struct Info
  {
    int row;
    decltype(it) iterator;
  };
  return Info{
      .row = static_cast<int>(std::distance(intervals.begin(), it)),
      .iterator = it,
  };
}

[[nodiscard]] bool is_match(const Project* project, const std::optional<QString>& name)
{
  if (project == nullptr) {
    return false;
  }
  if (name.has_value() && *name != project->name()) {
    return false;
  }
  return true;
}

}  // namespace

IntervalModel::IntervalModel(std::deque<std::unique_ptr<Interval>> intervals) : m_intervals(std::move(intervals))
{
}

int IntervalModel::rowCount(const QModelIndex& parent) const
{
  return static_cast<int>(m_intervals.size());
}

int IntervalModel::columnCount(const QModelIndex& parent) const
{
  return 5;
}

QVariant IntervalModel::data(const QModelIndex& index, const int role) const
{
  if (!index.isValid()) {
    return {};
  }

  const auto& interval = m_intervals.at(index.row());
  const auto* project = interval->project();
  if (role == Qt::BackgroundRole) {
    return project ? project->color() : QVariant{};
  }
  if (role == Qt::ForegroundRole) {
    return project ? ::contrast_color(project->color()) : QVariant{};
  }

  if (role == Qt::DisplayRole) {
    switch (index.column()) {
    case project_column:
      return project ? project->name() : QVariant{};
    case begin_column:
      return interval->begin().time();
    case end_column:
      return interval->end().time();
    case date_column:
      return QVariant::fromValue(DatePair{interval->begin().date(), interval->end().date()});
    case duration_column:
      return interval->duration_text();
    default:
      return {};
    }
  }

  if (role == Qt::EditRole) {
    switch (index.column()) {
    case project_column:
      return project ? project->name() : QVariant{};
    case begin_column:
      return interval->begin();
    case end_column:
      [[fallthrough]];
    case date_column:
      return interval->end();
    case duration_column:
      using std::chrono_literals::operator""min;
      return static_cast<int>(interval->duration() / 1min);
    default:
      return {};
    }
  }

  return {};
}

QVariant IntervalModel::headerData(const int, const Qt::Orientation, const int) const
{
  return {};
}

Qt::ItemFlags IntervalModel::flags(const QModelIndex& index) const
{
  if (!index.isValid()) {
    return {};
  }
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QModelIndex IntervalModel::index(const Interval& interval) const
{
  static constexpr auto column = 0;
  return index(::find(m_intervals, interval).row, column, {});
}

Interval& IntervalModel::remove_const(const Interval& interval) const
{
  return **::find(m_intervals, interval).iterator;
}

void IntervalModel::add(std::unique_ptr<Interval> interval)
{
  const auto row = static_cast<int>(m_intervals.size());
  beginInsertRows({}, row, row);
  m_intervals.emplace_back(std::move(interval));
  endInsertRows();
  Q_EMIT data_changed();
}

void IntervalModel::split_interval(const Interval& interval, const QDateTime& split_point)
{
  const auto location = ::find(m_intervals, interval);
  auto& left_interval = **location.iterator;
  beginInsertRows({}, location.row, location.row);
  const auto& right_interval =
      *m_intervals.emplace(std::next(location.iterator), std::make_unique<Interval>(interval.project()));
  endInsertRows();
  right_interval->swap_end(left_interval.end());
  left_interval.swap_end(split_point);
  right_interval->swap_begin(split_point);
}

std::unique_ptr<Interval> IntervalModel::extract(const Interval& interval)
{
  const auto location = ::find(m_intervals, interval);
  beginRemoveRows({}, location.row, location.row);
  auto extracted_interval = std::move(*location.iterator);
  m_intervals.erase(location.iterator);
  endRemoveRows();
  Q_EMIT data_changed();
  return extracted_interval;
}

void IntervalModel::set_intervals(std::deque<std::unique_ptr<Interval>> intervals)
{
  beginResetModel();
  m_intervals = std::move(intervals);
  endResetModel();
}

std::vector<Interval*> IntervalModel::intervals() const
{
  auto view = m_intervals | std::views::transform(&std::unique_ptr<Interval>::get);
  return std::vector(view.begin(), view.end());
}
std::vector<Interval*> IntervalModel::intervals(const Period& period) const
{
  std::vector<Interval> intervals;
  auto view =
      m_intervals
      | std::views::filter([&period](const auto& interval) { return period.contains(interval->begin().date()); })
      | std::views::transform(&std::unique_ptr<Interval>::get);
  return std::vector(view.begin(), view.end());
}

const Interval* IntervalModel::interval(const std::size_t index) const
{
  return m_intervals.at(index).get();
}

std::vector<Interval*> IntervalModel::open_intervals() const
{
  auto view = m_intervals | std::views::filter([](const auto& interval) { return !interval->end().isValid(); })
              | std::views::transform(&std::unique_ptr<Interval>::get);
  return std::vector(view.begin(), view.end());
}

std::chrono::minutes IntervalModel::minutes(const std::optional<Period>& period,
                                            const std::optional<QString>& name) const
{
  const auto accumulate_minutes_in_period = [period, &name](const std::chrono::minutes accu, const auto& interval) {
    if (!is_match(interval->project(), name)) {
      return accu;
    }
    if (period.has_value()) {
      return accu + period->overlap(*interval);
    }
    return accu + interval->duration();
  };
  using std::chrono_literals::operator""min;
  return std::accumulate(m_intervals.begin(), m_intervals.end(), 0min, accumulate_minutes_in_period);
}

std::chrono::minutes IntervalModel::minutes(const QDate& date, const std::optional<QString>& name) const
{
  return minutes(Period(date, Period::Type::Day), name);
}
