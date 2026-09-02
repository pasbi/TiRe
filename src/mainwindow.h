#pragma once

#include "application.h"
#include "intervalmodel.h"
#include "period.h"
#include <QActionGroup>
#include <QMainWindow>
#include <memory>
#include <set>

class QLabel;
class TimeSheet;
class UndoStack;

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  /**
   * @brief Takes the timesheet the window will display.
   * Requiring the data up front makes "the models exist before the views are wired" a property of
   * the type rather than a call-order convention.
   */
  explicit MainWindow(std::unique_ptr<TimeSheet> time_sheet);
  ~MainWindow() override;

  void next();
  void previous();
  void today();
  void set_date(const QDate& date);
  void set_period_type(Period::Type type);
  void set_period(const Period& period);

protected:
  void closeEvent(QCloseEvent* event) override;

Q_SIGNALS:
  void period_changed(Period period);

private:
  std::unique_ptr<Ui::MainWindow> m_ui;
  std::unique_ptr<TimeSheet> m_time_sheet;
  QActionGroup m_view_action_group;

  void set_time_sheet(std::unique_ptr<TimeSheet> time_sheet);
  void end_task();
  void switch_task();
  void edit_plan_settings();
  void update_window_title();

  /**
   * @name Persistence failure reporting
   * A silent write failure would be data loss, so a failure is shown three ways: logged, made
   * permanently visible in the status bar, and raised once in a dialog.
   * @{
   */
  void on_write_failed(const QString& message);
  void on_write_succeeded();
  /** @} */

  /** @brief Only blocks closing while writes are failing; there is nothing to save otherwise. */
  [[nodiscard]] bool can_close();

  Period m_current_period;
  QLabel* m_persistence_status_label = nullptr;
  bool m_persistence_failed = false;
};
