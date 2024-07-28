#include "periodsummary.h"
#include "intervalmodel.h"
#include "plan.h"
#include "ui_periodsummary.h"
#include <QSortFilterProxyModel>
#include <spdlog/spdlog.h>

namespace
{

[[nodiscard]] auto format_minutes(const std::chrono::minutes minutes)
{
  using std::chrono_literals::operator""h;
  using std::chrono_literals::operator""min;
  static constexpr auto field_width = 2;
  static constexpr auto base = 10;
  static constexpr auto fill_char = QChar('0');
  return QString{"%1:%2"}
      .arg(minutes / 1h, field_width, base, fill_char)
      .arg((minutes / 1min) % (1h / 1min), field_width, base, fill_char);
}

}  // namespace

class PeriodSummary::ProxyModel final : public QSortFilterProxyModel
{
public:
  using QSortFilterProxyModel::QSortFilterProxyModel;

  void set_source_model(IntervalModel* const model)
  {
    m_interval_model = model;
    setSourceModel(model);
  }

  void set_period(const Period& period)
  {
    m_period = period;
    invalidate();
  }

protected:
  [[nodiscard]] bool filterAcceptsRow(const int source_row, const QModelIndex& source_parent) const override
  {
    if (m_interval_model == nullptr) {
      return false;
    }
    const auto* const interval = m_interval_model->intervals().at(source_row);
    return m_period.contains(interval->begin().date(),
                             (interval->end().isValid() ? interval->end() : interval->begin()).date());
  }

private:
  const IntervalModel* m_interval_model = nullptr;
  Period m_period;
};

PeriodSummary::PeriodSummary(QWidget* parent)
  : QWidget(parent)
  , m_ui(std::make_unique<Ui::PeriodSummary>())
  , m_proxy_model(std::make_unique<PeriodSummary::ProxyModel>())
{
  m_ui->setupUi(this);
  connect(m_ui->tableView, &QAbstractItemView::doubleClicked, this,
          [this](const QModelIndex& index) { Q_EMIT double_clicked(m_proxy_model->mapToSource(index)); });
  m_ui->tableView->setModel(m_proxy_model.get());
}

PeriodSummary::~PeriodSummary() = default;

void PeriodSummary::set_period_type(const Period::Type type)
{
  m_type = type;
  set_date(QDate::currentDate());
}

void PeriodSummary::set_date(const QDate& date)
{
  m_current_period = Period(date, m_type);
  m_proxy_model->set_period(m_current_period);
  m_ui->lb_current_period->setText(m_current_period.label());
  recalculate();
}

void PeriodSummary::set_model(IntervalModel& interval_model, const Plan& plan)
{
  m_plan = &plan;
  m_interval_model = &interval_model;
  recalculate();
  if (m_interval_model != nullptr) {
    connect(m_interval_model, &IntervalModel::data_changed, this, &PeriodSummary::recalculate);
  }
}
void PeriodSummary::next()
{
  set_date(m_current_period.end().addDays(1));
}

void PeriodSummary::prev()
{
  set_date(m_current_period.begin().addDays(-1));
}

void PeriodSummary::today()
{
  set_date(QDate::currentDate());
}

void PeriodSummary::clear() const
{
  m_ui->lb_holiday->setText("-");
  m_ui->lb_total->setText("-");
  m_ui->lb_overtime->setText("-");
  m_ui->lb_overtime_cum->setText("-");
  m_ui->lb_planned->setText("-");
  m_ui->lb_sick->setText("-");
}

void PeriodSummary::recalculate()
{
  m_proxy_model->set_source_model(m_interval_model);
  if (m_interval_model == nullptr) {
    clear();
    return;
  }

  const auto actual_working_time = m_interval_model->minutes(m_current_period, Project::Type::Work);
  const auto planned_working_time = m_plan->planned_working_time(m_current_period.begin(), m_current_period.end());
  m_ui->lb_total->setText(::format_minutes(actual_working_time));
  m_ui->lb_holiday->setText(::format_minutes(m_interval_model->minutes(m_current_period, Project::Type::Holiday)));
  m_ui->lb_sick->setText(::format_minutes(m_interval_model->minutes(m_current_period, Project::Type::Sick)));
  m_ui->lb_planned->setText(::format_minutes(planned_working_time));
  m_ui->lb_overtime->setText(::format_minutes(actual_working_time - planned_working_time));

  const auto total_overtime = m_plan->overtime_offset() + m_interval_model->minutes({}, Project::Type::Work)
                              - m_plan->planned_working_time({}, QDate::currentDate());
  m_ui->lb_overtime_cum->setText(::format_minutes(total_overtime));

  update();
}
