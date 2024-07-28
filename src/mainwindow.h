#pragma once

#include "intervalmodel.h"
#include <QMainWindow>
#include <filesystem>
#include <memory>

class TimeSheet;

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

  void load();
  void load(std::filesystem::path filename);
  void save();
  void save_as();

private:
  std::unique_ptr<Ui::MainWindow> m_ui;
  std::unique_ptr<TimeSheet> m_time_sheet;
  void edit_date_time(const QModelIndex& index) const;
  void edit_project(const QModelIndex& index) const;
  std::filesystem::path m_filename;
};
