#include "views/planview.h"

#include "application.h"
#include "fmt.h"
#include "intervalmodel.h"
#include "plan.h"
#include "timesheet.h"
#include "ui_planview.h"

int PlanView::m_max_period_text_width = 0;

namespace
{

[[nodiscard]] auto format_minutes(const std::chrono::minutes minutes)
{
  using std::chrono_literals::operator""h;
  using std::chrono_literals::operator""min;
  static constexpr auto field_width = 2;
  static constexpr auto base = 10;
  static constexpr auto fill_char = QChar('0');
  return QString{"%1%2:%3"}
      .arg(minutes < 0min ? "-" : "")
      .arg(std::abs(minutes / 1h), field_width, base, fill_char)
      .arg(std::abs((minutes / 1min)) % (1h / 1min), field_width, base, fill_char);
}

}  // namespace

PlanView::PlanView(QWidget* parent) : AbstractPeriodView(parent), m_ui(std::make_unique<Ui::PlanView>())
{
  m_ui->setupUi(this);
  clear();
}

PlanView::~PlanView() = default;

void PlanView::clear() const
{
  for (auto* const lb : {m_ui->lb_holiday, m_ui->lb_total_balance, m_ui->lb_actual_worktime, m_ui->lb_balance_carryover,
                         m_ui->lb_expected_worktime, m_ui->lb_period, m_ui->lb_period, m_ui->lb_sick,
                         m_ui->lb_period_balance, m_ui->lb_total_worktime})
  {
    lb->setText("-");
    lb->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
  }
}

void PlanView::invalidate()
{
  if (time_sheet() == nullptr) {
    clear();
    return;
  }

  const auto& plan = time_sheet()->plan();
  const Period current_period(std::max(this->current_period().begin(), plan.start()),
                              std::min(this->current_period().end(), Application::current_date_time().date()));

  const auto& interval_model = time_sheet()->interval_model();
  const auto actual_working_time = interval_model.minutes(current_period, Project::Type::Work);
  const auto sick_time = interval_model.minutes(current_period, Project::Type::Sick);
  const auto holiday_time = interval_model.minutes(current_period, Project::Type::Holiday);
  const auto total_working_time = plan.planned_working_time(current_period);
  const auto expected_working_time = total_working_time - sick_time - holiday_time;
  const auto balance = actual_working_time - expected_working_time;
  const Period total_period{plan.start(), current_period.end()};
  const auto total_balance =
      plan.overtime_offset() + interval_model.minutes(total_period) - plan.planned_working_time(total_period);
  const auto balance_carryover = total_balance - balance;

  m_ui->lb_period->setText(period_text(current_period));
  m_ui->lb_period->setToolTip(
      tr("From %1 to %2").arg(current_period.begin().toString()).arg(current_period.end().toString()));
  m_ui->lb_total_worktime->setText(::format_minutes(total_working_time));
  m_ui->lb_expected_worktime->setText(::format_minutes(expected_working_time));
  m_ui->lb_sick->setText(::format_minutes(sick_time));
  m_ui->lb_holiday->setText(::format_minutes(holiday_time));
  m_ui->lb_actual_worktime->setText(::format_minutes(actual_working_time));
  m_ui->lb_balance_carryover->setText(::format_minutes(balance_carryover));
  m_ui->lb_balance_carryover->setToolTip(
      tr("The balance from before this period (since %1)").arg(plan.start().toString()));
  m_ui->lb_period_balance->setText(::format_minutes(balance));
  m_ui->lb_total_balance->setText(::format_minutes(total_balance));
  m_ui->lb_total_balance->setToolTip(
      tr("The balance since the beginning of records (including this period, from %1 to %2).")
          .arg(plan.start().toString(), current_period.end().toString()));

  update();
}
QSize PlanView::sizeHint() const
{
  // The PlanView can only grow.
  // It's difficult to calculate the maximum required size in advance.
  // Instead, remember the largest required size until now and don't shrink it.
  // That should reduce wobbling around when browsing history.
  auto size_hint = AbstractPeriodView::sizeHint();
  m_max_period_text_width = std::max(m_max_period_text_width, size_hint.width());
  size_hint.setWidth(m_max_period_text_width);
  return size_hint;
}

QString PlanView::period_text(const Period& period) const
{
  return current_period().label() + (current_period().dates() == period.dates() ? "" : "*");
}
