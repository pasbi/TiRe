#pragma once

#include "application.h"
#include "intervalmodel.h"
#include "period.h"
#include <QActionGroup>
#include <QMainWindow>
#include <filesystem>
#include <memory>

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
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;
  void set_time_sheet(std::unique_ptr<TimeSheet> time_sheet);
  void set_filename(std::filesystem::path filename);

  bool load();
  bool load(std::filesystem::path filename);
  bool save();
  bool save_as();
  bool new_time_sheet();

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
  std::filesystem::path m_filename;
  QActionGroup m_view_action_group;

  void delete_selected_intervals() const;
  void split_selected_intervals() const;
  void init_context_menu_actions();
  void show_table_context_menu(const QPoint& pos);
  void edit_date_time(const QModelIndex& index) const;
  void edit_project(const QModelIndex& index) const;
  void end_task();
  void switch_task();
  std::unique_ptr<UndoStack> m_undo_stack;
  void update_window_title();

  std::vector<std::unique_ptr<QAction>> m_context_menu_actions;

  [[nodiscard]] bool can_close();
  Period m_current_period = Period(Application::current_date_time().date(), Period::Type::Day);
};
