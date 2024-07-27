#include "model.h"
#include "json.h"
#include "period.h"
#include <QColor>
#include <nlohmann/json.hpp>

namespace
{

constexpr auto intervals_key = "intervals";
constexpr auto projects_key = "projects";

[[nodiscard]] QString format_date(const QDate& begin, const QDate& end)
{
  static constexpr auto format = "ddd, dd.";
  if (begin == end) {
    return begin.toString(format);
  }
  return begin.toString(format) + " - " + end.toString(format);
}

}  // namespace

nlohmann::json Model::serialize() const
{
  return {
      {::intervals_key, m_intervals},
      {::projects_key, m_projects},
  };
}

void Model::deserialize(const nlohmann::json& data)
{
  set_intervals(data[intervals_key]);
  m_projects = data.value(projects_key, QStringList{});
}

int Model::rowCount(const QModelIndex& parent) const
{
  return static_cast<int>(m_intervals.size());
}

int Model::columnCount(const QModelIndex& parent) const
{
  return 5;
}

QVariant Model::data(const QModelIndex& index, const int role) const
{
  if (!index.isValid()) {
    return {};
  }

  if (role == Qt::BackgroundRole) {
    return background_data(index);
  }

  const auto& interval = m_intervals.at(index.row());
  if (role == Qt::DisplayRole) {
    switch (index.column()) {
    case project_column:
      return interval.project();
    case begin_column:
      return interval.begin().time();
    case end_column:
      return interval.end().time();
    case date_column:
      return ::format_date(interval.begin().date(), interval.end().date());
    case duration_column:
      return interval.duration_text();
    default:
      return {};
    }
  }

  if (role == Qt::EditRole) {
    switch (index.column()) {
    case project_column:
      return interval.project();
    case begin_column:
      return interval.begin();
    case end_column:
      return interval.end();
    default:
      return {};
    }
  }

  return {};
}

QVariant Model::headerData(const int, const Qt::Orientation, const int) const
{
  return {};
}

Qt::ItemFlags Model::flags(const QModelIndex& index) const
{
  if (!index.isValid()) {
    return {};
  }
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool Model::setData(const QModelIndex& index, const QVariant& value, const int role)
{
  if (role != Qt::EditRole) {
    return false;
  }

  auto& interval = m_intervals.at(index.row());
  const auto emit_data_changed = [left = index.siblingAtColumn(0), right = index.siblingAtColumn(columnCount({}) - 1),
                                  this]() { Q_EMIT dataChanged(left, right); };
  switch (index.column()) {
  case begin_column:
    interval.set_begin(value.toDateTime());
    emit_data_changed();
    return true;
  case end_column:
    interval.set_end(value.toDateTime());
    emit_data_changed();
    return true;
  case project_column:
    set_project(interval, value.toString());
    emit_data_changed();
    return true;
  default:
    return false;
  }
}

void Model::new_interval()
{
  Interval interval;
  interval.set_begin(QDateTime::currentDateTime());
  add_interval(std::move(interval));
}

void Model::add_interval(Interval interval)
{
  const auto row = static_cast<int>(m_intervals.size());
  beginInsertRows({}, row, row);
  m_intervals.emplace_back(std::move(interval));
  endInsertRows();
}

const QStringList& Model::projects() const noexcept
{
  return m_projects;
}

void Model::set_intervals(std::deque<Interval> intervals)
{
  beginResetModel();
  m_intervals = std::move(intervals);
  endResetModel();
}

const std::deque<Interval>& Model::intervals() const noexcept
{
  return m_intervals;
}

void Model::set_project(Interval& interval, QString project)
{
  if (!m_projects.contains(project)) {
    m_projects.append(project);
  }
  interval.set_project(std::move(project));
}

QVariant Model::background_data(const QModelIndex& index) const
{
  if (!index.isValid()) {
    return {};
  }

  switch (m_intervals.at(index.row()).begin().date().dayOfWeek()) {
  case 0:
    return QColor{0xFF808080};
  case Qt::Monday:
    return QColor{0xFF54DFDA};
  case Qt::Tuesday:
    return QColor{0xFF4FE056};
  case Qt::Wednesday:
    return QColor{0xFF606BE0};
  case Qt::Thursday:
    return QColor{0xFFE0DE4F}.darker();
  case Qt::Friday:
    return QColor{0xFFC255DF};
  case Qt::Saturday:
    return QColor{0xFFE0B156}.darker();
  case Qt::Sunday:
    return QColor{0xFFC25144};
  default:
    return {};
  }
}

std::chrono::minutes Model::minutes_worked(const Period& period, const QString& project) const
{
  using std::chrono_literals::operator""min;
  const auto accumulate_minutes_in_period = [period, &project](const std::chrono::minutes accu,
                                                               const Interval& interval) {
    return accu + (project.isEmpty() || interval.project() == project ? period.minutes_overlap(interval) : 0min);
  };
  return std::accumulate(m_intervals.begin(), m_intervals.end(), 0min, accumulate_minutes_in_period);
}
