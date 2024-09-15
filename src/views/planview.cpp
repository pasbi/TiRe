#include "views/planview.h"

#include "application.h"
#include "intervalmodel.h"
#include "plan.h"
#include "timesheet.h"
#include "ui_planview.h"

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
}

PlanView::~PlanView() = default;

void PlanView::clear() const
{
  m_ui->lb_holiday->setText("-");
  m_ui->lb_total->setText("-");
  m_ui->lb_overtime->setText("-");
  m_ui->lb_overtime_cum->setText("-");
  m_ui->lb_planned->setText("-");
  m_ui->lb_sick->setText("-");
}

void PlanView::invalidate()
{
  if (time_sheet() == nullptr) {
    clear();
    return;
  }

  const auto& interval_model = time_sheet()->interval_model();
  const auto& plan = time_sheet()->plan();
  const auto actual_working_time = interval_model.minutes(current_period(), Project::Type::Work);
  const auto actual_leave_time = interval_model.minutes(current_period(), Project::Type::Holiday)
                                 + interval_model.minutes(current_period(), Project::Type::Sick);
  const auto planned_working_time = plan.planned_working_time(current_period().begin(), current_period().end());
  m_ui->lb_total->setText(::format_minutes(actual_working_time));
  m_ui->lb_holiday->setText(::format_minutes(interval_model.minutes(current_period(), Project::Type::Holiday)));
  m_ui->lb_sick->setText(::format_minutes(interval_model.minutes(current_period(), Project::Type::Sick)));
  m_ui->lb_planned->setText(::format_minutes(planned_working_time));
  m_ui->lb_overtime->setText(::format_minutes(actual_working_time - planned_working_time + actual_leave_time));

  const auto total_overtime = plan.overtime_offset() + interval_model.minutes({}, Project::Type::Work)
                              - plan.planned_working_time({}, Application::current_date_time().date());
  m_ui->lb_overtime_cum->setText(::format_minutes(total_overtime));

  update();
}
