#include "periodsummary.h"
#include "model.h"
#include "ui_periodsummary.h"

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

PeriodSummary::PeriodSummary(QWidget* parent) : QWidget(parent), m_ui(std::make_unique<Ui::PeriodSummary>())
{
  m_ui->setupUi(this);
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
  m_ui->lb_current_period->setText(m_current_period.label());
  recalculate();
}

void PeriodSummary::set_model(const Model& model)
{
  m_model = &model;
  recalculate();
  if (m_model != nullptr) {
    connect(m_model, &Model::dataChanged, this, &PeriodSummary::recalculate);
    connect(m_model, &Model::modelReset, this, &PeriodSummary::recalculate);
    connect(m_model, &Model::rowsInserted, this, &PeriodSummary::recalculate);
    connect(m_model, &Model::rowsRemoved, this, &PeriodSummary::recalculate);
  }
}

void PeriodSummary::clear()
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
  if (m_model == nullptr) {
    clear();
    return;
  }

  m_ui->lb_total->setText(format_minutes(m_model->minutes_worked(m_current_period)));
  update();
}
