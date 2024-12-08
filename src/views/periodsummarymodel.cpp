#include "views/periodsummarymodel.h"
#include "colorutil.h"
#include "intervalmodel.h"
#include "projectmodel.h"
#include "timesheet.h"
#include <QPalette>

class PeriodSummaryModel::Row
{
public:
  explicit Row(const PeriodSummaryModel& model) : m_model(model)
  {
  }

  virtual ~Row() = default;
  [[nodiscard]] virtual QVariant header_data(int role) = 0;
  [[nodiscard]] virtual QVariant data(int section, int role) = 0;
  [[nodiscard]] const PeriodSummaryModel& model() const noexcept
  {
    return m_model;
  }

private:
  const PeriodSummaryModel& m_model;
};

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

class ProjectRow final : public PeriodSummaryModel::Row
{
public:
  ProjectRow(const Project& project, const PeriodSummaryModel& model) : Row(model), m_project(project)
  {
  }

  [[nodiscard]] QVariant header_data(const int role) override
  {
    switch (role) {
    case Qt::DisplayRole:
      return m_project.name();
    case Qt::BackgroundRole:
      return m_project.color();
    case Qt::ForegroundRole:
      return ::contrast_color(m_project.color());
    default:
      return {};
    }
  }

  [[nodiscard]] QVariant data(const int section, const int role) override
  {
    const auto date = model().date(section);
    const auto duration = model().get_duration(date, &m_project);
    using std::chrono_literals::operator""min;
    switch (role) {
    case Qt::DisplayRole:
      return ::format_minutes(duration);
    case Qt::BackgroundRole:
      if (duration > 0min) {
        return m_project.color();
      }
      return ::background(date);
    case Qt::ForegroundRole:
      return duration == 0min ? QVariant{} : ::contrast_color(m_project.color());
    default:
      return {};
    }
  }

private:
  const Project& m_project;
};

class TotalExtraRow final : public PeriodSummaryModel::Row
{
public:
  using Row::Row;

  [[nodiscard]] QVariant header_data(const int role) override
  {
    if (role == Qt::DisplayRole) {
      return QObject::tr("TOTAL");
    }
    return {};
  }

  [[nodiscard]] QVariant data(const int section, const int role) override
  {
    if (role == Qt::DisplayRole) {
      return ::format_minutes(model().get_duration(model().date(section)));
    }
    return {};
  }
};

[[nodiscard]] auto make_rows(const PeriodSummaryModel& period_summary_model)
{
  auto rows = std::vector<std::unique_ptr<PeriodSummaryModel::Row>>();
  const auto* const project_model = period_summary_model.project_model();
  if (project_model == nullptr) {
    return rows;
  }

  auto projects = project_model->projects();
  std::ranges::sort(projects, std::ranges::less{}, &Project::name);

  rows.reserve(projects.size() + 1);
  rows.emplace_back(std::make_unique<TotalExtraRow>(period_summary_model));
  for (const auto& project : projects) {
    rows.emplace_back(std::make_unique<ProjectRow>(*project, period_summary_model));
  }
  return rows;
}

}  // namespace

PeriodSummaryModel::PeriodSummaryModel(QObject* parent) : QAbstractTableModel(parent)
{
  invalidate();
}

PeriodSummaryModel::~PeriodSummaryModel() = default;

int PeriodSummaryModel::rowCount(const QModelIndex& parent) const
{
  return static_cast<int>(m_rows.size());
}

int PeriodSummaryModel::columnCount(const QModelIndex& parent) const
{
  return m_time_sheet == nullptr ? 0 : m_period.days();
}

QVariant PeriodSummaryModel::data(const QModelIndex& index, const int role) const
{
  if (m_rows.empty() || !index.isValid()) {
    return {};
  }
  return m_rows.at(index.row())->data(index.column(), role);
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
    return m_rows.at(section)->header_data(role);
  }
  return {};
}

void PeriodSummaryModel::set_source(const TimeSheet* model)
{
  m_time_sheet = model;
  if (m_time_sheet != nullptr) {
    connect(&m_time_sheet->project_model(), &ProjectModel::projects_changed, this, &PeriodSummaryModel::invalidate);
  }
  invalidate();
  update_summary();
}

void PeriodSummaryModel::set_period(const Period& period)
{
  beginResetModel();
  m_period = period;
  update_summary();
  endResetModel();
}

std::vector<Project*> PeriodSummaryModel::projects() const
{
  if (m_time_sheet == nullptr) {
    return {};
  }
  return m_time_sheet->project_model().projects();
}

ProjectModel* PeriodSummaryModel::project_model() const noexcept
{
  if (m_time_sheet == nullptr) {
    return nullptr;
  }
  return &m_time_sheet->project_model();
}

QDate PeriodSummaryModel::date(const int column) const noexcept
{
  return m_period.begin().addDays(column);
}

void PeriodSummaryModel::invalidate()
{
  beginResetModel();
  m_rows = ::make_rows(*this);
  endResetModel();
}

void PeriodSummaryModel::update_summary()
{
  m_minutes.clear();
  if (m_time_sheet == nullptr) {
    return;
  }

  for (const auto* interval : m_time_sheet->interval_model().intervals(m_period)) {
    using std::chrono_literals::operator""min;
    auto& duration = m_minutes.try_emplace(interval->begin().date())
                         .first->second.try_emplace(interval->project(), 0min)
                         .first->second;
    duration += interval->duration();
  }
}

std::chrono::minutes PeriodSummaryModel::get_duration(const QDate& date, const Project* project) const
{
  const auto it_1 = m_minutes.find(date);
  using std::chrono_literals::operator""min;
  if (it_1 == m_minutes.end()) {
    return 0min;
  }
  if (project == nullptr) {
    return std::accumulate(it_1->second.begin(), it_1->second.end(), 0min,
                           [](const auto& accu, const auto& pair) { return accu + pair.second; });
  }
  const auto it_2 = it_1->second.find(project);
  if (it_2 == it_1->second.end()) {
    return 0min;
  }
  return it_2->second;
}
