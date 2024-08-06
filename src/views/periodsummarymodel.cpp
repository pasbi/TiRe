#include "views/periodsummarymodel.h"

#include "colorutil.h"
#include "intervalmodel.h"
#include "projectmodel.h"
#include "timesheet.h"
#include <qpalette.h>

namespace
{

[[nodiscard]] auto format_minutes(const std::chrono::minutes& minutes)
{
  using std::chrono_literals::operator""min;
  using std::chrono_literals::operator""h;

  static constexpr auto field_width = 2;
  static constexpr auto base = 10;
  static constexpr auto fill_char = QChar('0');

  if (minutes == 0min) {
    return QString{};
  }

  return QString("%1:%2")
      .arg(minutes / 1h, field_width, base, fill_char)
      .arg((minutes / 1min) % (1h / 1min), field_width, base, fill_char);
};
}  // namespace

int PeriodSummaryModel::rowCount(const QModelIndex& parent) const
{
  if (m_time_sheet == nullptr) {
    return 0;
  }

  return static_cast<int>(m_time_sheet->project_model().projects().size());
}

int PeriodSummaryModel::columnCount(const QModelIndex& parent) const
{
  if (m_time_sheet == nullptr) {
    return 0;
  }

  return m_period.days() + 1;
}

QVariant PeriodSummaryModel::data(const QModelIndex& index, int role) const
{
  using std::chrono_literals::operator""min;
  if (m_time_sheet == nullptr || !index.isValid()) {
    return {};
  }

  const auto get = [this](const Project& project, const QDate& date) {
    const auto it = m_minutes.find({&project, date});
    if (it == m_minutes.end()) {
      return 0min;
    }
    return it->second;
  };

  const auto* const project = this->project(index.row());
  const auto date = this->date(index.column());
  switch (role) {
  case Qt::DisplayRole:
    return ::format_minutes(get(*project, date));
  case Qt::BackgroundRole:
    if (get(*project, date) > 0min) {
      return project->color();
    }
    return ::background(date);
  case Qt::ForegroundRole:
    return get(*project, date) == 0min ? QVariant{} : ::contrast_color(project->color());
  default:
    return {};
  }
}

QVariant PeriodSummaryModel::headerData(const int section, const Qt::Orientation orientation, const int role) const
{
  if (m_time_sheet == nullptr) {
    return {};
  }

  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
    const auto date_format = [type = m_period.type()]() {
      switch (type) {
        using enum Period::Type;
      case Day:
        return tr("dddd, dd. MMM");
      case Week:
        return tr("ddd, dd.");
      case Month:
        return tr("dd.");
      case Year:
        [[fallthrough]];
      case Custom:
        return tr("dd.MM.");
      }
      return QString{};
    }();
    return date(section).toString(date_format);
  }
  if (orientation == Qt::Vertical) {
    const auto& project = *this->project(section);
    switch (role) {
    case Qt::DisplayRole:
      return project.label();
    case Qt::BackgroundRole:
      return project.color();
    case Qt::ForegroundRole:
      return ::contrast_color(project.color());
    default:
      return {};
    }
  }
  return {};
}

void PeriodSummaryModel::set_source(const TimeSheet* model)
{
  beginResetModel();
  m_time_sheet = model;
  update_summary();
  endResetModel();
}

void PeriodSummaryModel::set_period(const Period& period)
{
  beginResetModel();
  m_period = period;
  update_summary();
  endResetModel();
}

const Project* PeriodSummaryModel::project(const int row) const noexcept
{
  if (m_time_sheet == nullptr) {
    return nullptr;
  }
  return m_time_sheet->project_model().projects().at(row);
}

QDate PeriodSummaryModel::date(const int column) const noexcept
{
  return m_period.begin().addDays(column);
}

void PeriodSummaryModel::update_summary()
{
  m_minutes.clear();
  if (m_time_sheet == nullptr) {
    return;
  }

  for (const auto* interval : m_time_sheet->interval_model().intervals(m_period)) {
    using std::chrono_literals::operator""min;
    const auto [it, _] = m_minutes.try_emplace({&interval->project(), interval->begin().date()}, 0min);
    it->second += interval->duration();
  }
}
