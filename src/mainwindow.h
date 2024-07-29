#pragma once

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

  void load();
  void load(std::filesystem::path filename);
  void save();
  void save_as();

private:
  std::unique_ptr<Ui::MainWindow> m_ui;
  std::unique_ptr<TimeSheet> m_time_sheet;
  std::filesystem::path m_filename;
  QActionGroup m_view_action_group;

  void delete_selected_intervals() const;
  void split_selected_intervals() const;
  void show_table_context_menu(const QPoint& pos);
  void edit_date_time(const QModelIndex& index) const;
  void edit_project(const QModelIndex& index) const;
  void set_period_type(Period::Type type);
  std::unique_ptr<UndoStack> m_undo_stack;
  void update_window_title();
};
