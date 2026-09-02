#include "plansettingsdialog.h"

#include "application.h"
#include "commands/commands.h"
#include "commands/undostack.h"
#include "plan.h"

#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSpinBox>

namespace
{

// Wide enough for a few working years of accumulated correction in either direction.
constexpr auto overtime_offset_limit_minutes = 1'000'000;

}  // namespace

PlanSettingsDialog::PlanSettingsDialog(Plan& plan, QWidget* parent) : QDialog(parent), m_plan(plan)
{
  setWindowTitle(tr("Plan Settings"));

  m_start_edit = new QDateEdit{m_plan.start(), this};
  m_start_edit->setCalendarPopup(true);
  m_start_edit->setToolTip(tr("Overtime is accounted for from this date on."));

  m_overtime_offset_edit = new QSpinBox{this};
  m_overtime_offset_edit->setRange(-overtime_offset_limit_minutes, overtime_offset_limit_minutes);
  m_overtime_offset_edit->setSuffix(tr(" min"));
  m_overtime_offset_edit->setValue(static_cast<int>(m_plan.overtime_offset().count()));
  m_overtime_offset_edit->setToolTip(tr("A correction added to the computed overtime balance. May be negative."));

  auto* const buttons = new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this};
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  auto* const layout = new QFormLayout{this};
  layout->addRow(tr("Plan &start:"), m_start_edit);
  layout->addRow(tr("&Overtime offset:"), m_overtime_offset_edit);
  layout->addWidget(buttons);
}

PlanSettingsDialog::~PlanSettingsDialog() = default;

void PlanSettingsDialog::accept()
{
  const auto start = m_start_edit->date();
  const auto overtime_offset = std::chrono::minutes{m_overtime_offset_edit->value()};
  const auto start_changed = start != m_plan.start();
  const auto offset_changed = overtime_offset != m_plan.overtime_offset();

  if (start_changed || offset_changed) {
    // One macro, so changing both is a single undo step and a single transaction.
    const auto macro = Application::undo_stack().start_macro(tr("Change plan settings"));
    if (start_changed) {
      Application::undo_stack().push(make_modify_plan_start_command(m_plan, start));
    }
    if (offset_changed) {
      Application::undo_stack().push(make_modify_plan_overtime_offset_command(m_plan, overtime_offset));
    }
  }
  QDialog::accept();
}
