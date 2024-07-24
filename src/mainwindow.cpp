#include "mainwindow.h"

#include "datetimeeditor.h"
#include "model.h"
#include "projecteditor.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent), m_ui(std::make_unique<Ui::MainWindow>()), m_model(std::make_unique<Model>())
{
  m_ui->setupUi(this);
  m_ui->tableView->setModel(m_model.get());
  connect(m_ui->action_Add_Interval, &QAction::triggered, m_model.get(), &Model::new_interval);
  connect(m_ui->tableView, &QAbstractItemView::doubleClicked, this, [this](const QModelIndex& index) {
    if (index.column() == Model::begin_column || index.column() == Model::end_column) {
      edit_date_time(index);
    } else if (index.column() == Model::project_column) {
      edit_project(index);
    }
  });
}

MainWindow::~MainWindow() = default;

void MainWindow::edit_date_time(const QModelIndex& index) const
{
  DateTimeEditor e;
  const auto old_date_time = m_model->data(index, Qt::EditRole).toDateTime();
  e.set_date(old_date_time.date());
  e.set_time(old_date_time.time());
  if (e.exec() == QDialog::Accepted) {
    m_model->setData(index, e.date_time(), Qt::EditRole);
  }
}

void MainWindow::edit_project(const QModelIndex& index) const
{
  ProjectEditor e(m_model->projects());
  e.set_project(m_model->data(index, Qt::EditRole).toString());
  if (e.exec() == QDialog::Accepted) {
    m_model->setData(index, e.project_name(), Qt::EditRole);
  }
}
