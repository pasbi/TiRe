#include "model.h"
#include <nlohmann/json.hpp>

namespace
{

constexpr auto intervals_key = "intervals";

}  // namespace

nlohmann::json Model::serialize() const
{
  return {
      {intervals_key, m_intervals},
  };
}

void Model::deserialize(const nlohmann::json& data)
{
  set_intervals(data[intervals_key]);
}

int Model::rowCount(const QModelIndex& parent) const
{
  return static_cast<int>(m_intervals.size());
}

int Model::columnCount(const QModelIndex& parent) const
{
  return 4;
}

QVariant Model::data(const QModelIndex& index, const int role) const
{
  if (!index.isValid() || (role != Qt::DisplayRole && role != Qt::EditRole)) {
    return {};
  }

  const auto& interval = m_intervals.at(index.row());
  switch (index.column()) {
  case project_column:
    return interval.project();
  case begin_column:
    return interval.begin();
  case end_column:
    return interval.end();
  case duration_column:
    return interval.duration_text();
  default:
    return {};
  }
}

QVariant Model::headerData(const int section, const Qt::Orientation orientation, const int role) const
{
  if (role != Qt::DisplayRole) {
    return {};
  }
  if (orientation != Qt::Horizontal) {
    return {};
  }
  switch (section) {
  case begin_column:
    return tr("begin");
  case end_column:
    return tr("end");
  case project_column:
    return tr("project");
  case duration_column:
    return tr("duration");
  default:
    return {};
  }
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
