#include "periodsummary.h"
#include "ui_periodsummary.h"

PeriodSummary::PeriodSummary(QWidget* parent) : QWidget(parent), m_ui(std::make_unique<Ui::PeriodSummary>())
{
  m_ui->setupUi(this);
}

PeriodSummary::~PeriodSummary() = default;

void PeriodSummary::set_period_type(const Period::Type type)
{
  m_type = type;
}

void PeriodSummary::set_date(const QDate& date)
{
  const auto current_period = Period(date, m_type);
  // m_ui->lb_current_period = current_period.label();
  // fill labels...
}
