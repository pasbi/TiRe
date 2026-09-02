#pragma once

#include <QDialog>
#include <memory>

class Plan;
class QDateEdit;
class QSpinBox;

/**
 * @class PlanSettingsDialog plansettingsdialog.h "plansettingsdialog.h"
 * @brief Edits the plan's start date and overtime offset.
 *
 * These two values used to be reachable only by hand-editing the JSON timesheet. With the store
 * moved to a database that is no longer possible, so they get a proper dialog. Both edits go
 * through the undo stack, and therefore through the same write-through path as everything else.
 */
class PlanSettingsDialog : public QDialog
{
  Q_OBJECT
public:
  explicit PlanSettingsDialog(Plan& plan, QWidget* parent);
  ~PlanSettingsDialog() override;

  void accept() override;

private:
  Plan& m_plan;
  QDateEdit* m_start_edit = nullptr;
  QSpinBox* m_overtime_offset_edit = nullptr;
};
