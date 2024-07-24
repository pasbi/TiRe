#pragma once

#include "model.h"

#include <QMainWindow>
#include <memory>

class Model;

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

private:
  std::unique_ptr<Ui::MainWindow> m_ui;
  std::unique_ptr<Model> m_model;
  void edit_date_time(const QModelIndex& index) const;
  void edit_project(const QModelIndex& index) const;
};
