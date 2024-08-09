#include "views/periodsummarymodel.h"
#include "colorutil.h"
#include "intervalmodel.h"
#include "projectmodel.h"
#include "timesheet.h"
#include <QPalette>

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

class PeriodSummaryModel::ExtraRow
{
public:
  virtual ~ExtraRow() = default;
  [[nodiscard]] virtual QVariant header_data(int role) = 0;
  [[nodiscard]] virtual QVariant data(int section, int role) = 0;
};

class TotalExtraRow final : public PeriodSummaryModel::ExtraRow
{
public:
  TotalExtraRow(const PeriodSummaryModel& model) : m_model(model)
  {
  }

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
      return ::format_minutes(m_model.get_duration(m_model.date(section)));
    }
    return {};
  }

private:
  const PeriodSummaryModel& m_model;
};

namespace
{

auto make_extra_rows(const PeriodSummaryModel& period_summary_model)
{
  auto rows = std::vector<std::unique_ptr<PeriodSummaryModel::ExtraRow>>();
  rows.emplace_back(std::make_unique<TotalExtraRow>(period_summary_model));
  return rows;
}

}  // namespace

PeriodSummaryModel::PeriodSummaryModel(QObject* parent)
  : QAbstractTableModel(parent), m_extra_rows(::make_extra_rows(*this))
{
}

PeriodSummaryModel::~PeriodSummaryModel() = default;

int PeriodSummaryModel::rowCount(const QModelIndex& parent) const
{
  if (m_time_sheet == nullptr) {
    return 0;
  }

  return static_cast<int>(m_time_sheet->project_model().projects().size() + m_extra_rows.size());
}

int PeriodSummaryModel::columnCount(const QModelIndex& parent) const
{
  return m_time_sheet == nullptr ? 0 : m_period.days();
}

QVariant PeriodSummaryModel::data(const QModelIndex& index, const int role) const
{
  using std::chrono_literals::operator""min;
  if (m_time_sheet == nullptr || !index.isValid()) {
    return {};
  }

  const auto date = this->date(index.column());
  if (index.row() < m_extra_rows.size()) {
    return m_extra_rows.at(index.row())->data(index.column(), role);
  }

  const auto* const project = this->project(index.row() - static_cast<int>(m_extra_rows.size()));
  switch (role) {
  case Qt::DisplayRole:
    return ::format_minutes(get_duration(date, project));
  case Qt::BackgroundRole:
    if (get_duration(date, project) > 0min) {
      return project->color();
    }
    return ::background(date);
  case Qt::ForegroundRole:
    return get_duration(date, project) == 0min ? QVariant{} : ::contrast_color(project->color());
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
    if (section < m_extra_rows.size()) {
      return m_extra_rows.at(section)->header_data(role);
    }
    const auto& project = *this->project(section - m_extra_rows.size());
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
    auto& duration = m_minutes.try_emplace(interval->begin().date())
                         .first->second.try_emplace(&interval->project(), 0min)
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
