#include "mainwindow.h"
#include "datetimeeditor.h"
#include "exceptions.h"
#include "intervalmodel.h"
#include "period.h"
#include "projecteditor.h"
#include "projectmodel.h"
#include "serialization.h"
#include "timesheet.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <fmt/chrono.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace
{

constexpr auto extension = ".ts";
[[nodiscard]] auto file_filter()
{
  return QObject::tr("Time Sheets (*%1)").arg(extension);
}

}  // namespace

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent), m_ui(std::make_unique<Ui::MainWindow>()), m_time_sheet(std::make_unique<TimeSheet>())
{
  m_ui->setupUi(this);
  connect(m_ui->tableView, &QAbstractItemView::doubleClicked, this, [this](const QModelIndex& index) {
    if (index.column() == IntervalModel::begin_column || index.column() == IntervalModel::end_column) {
      edit_date_time(index);
    } else if (index.column() == IntervalModel::project_column) {
      edit_project(index);
    }
  });
  connect(m_ui->action_Load, &QAction::triggered, this, QOverload<>::of(&MainWindow::load));
  connect(m_ui->action_Save, &QAction::triggered, this, &MainWindow::save);
  connect(m_ui->action_Save_As, &QAction::triggered, this, &MainWindow::save_as);

  // TODO unify them in one widget
  m_ui->tab_day->set_period_type(Period::Type::Day);
  m_ui->tab_week->set_period_type(Period::Type::Week);
  m_ui->tab_month->set_period_type(Period::Type::Month);
  m_ui->tab_year->set_period_type(Period::Type::Year);

  set_time_sheet(std::make_unique<TimeSheet>());
}

MainWindow::~MainWindow() = default;

void MainWindow::set_time_sheet(std::unique_ptr<TimeSheet> time_sheet)
{
  m_time_sheet = std::move(time_sheet);
  m_ui->tableView->setModel(&m_time_sheet->interval_model());
  connect(m_ui->action_Add_Interval, &QAction::triggered, this,
          [this]() { m_time_sheet->interval_model().new_interval(m_time_sheet->project_model().empty_project()); });

  m_ui->tab_day->set_model(m_time_sheet->interval_model());
  m_ui->tab_week->set_model(m_time_sheet->interval_model());
  m_ui->tab_month->set_model(m_time_sheet->interval_model());
  m_ui->tab_year->set_model(m_time_sheet->interval_model());
}

void MainWindow::load()
{
  const auto last_load_dir = QDir::home().path();  // TODO
  const auto q_filename =
      QFileDialog::getOpenFileName(this, QApplication::applicationDisplayName(), last_load_dir, file_filter());
  if (q_filename.isEmpty()) {
    return;
  }

  load(static_cast<std::filesystem::path>(q_filename.toStdString()));
}

void MainWindow::load(std::filesystem::path filename)
{
  try {
    std::ifstream ifs(filename);
    if (!ifs) {
      QMessageBox::critical(this, QApplication::applicationDisplayName(),
                            tr("Failed to open '%1' for reading.").arg(QString::fromStdString(m_filename.string())));
    }
    nlohmann::json data;
    ifs >> data;
    set_time_sheet(::deserialize(data));
    m_filename = std::move(filename);
  } catch (const DeserializationError& e) {
    QMessageBox::critical(
        this, QApplication::applicationDisplayName(),
        tr("Failed to open '%1': %2").arg(QString::fromStdString(filename.string()), QString::fromStdString(e.what())));
  }
}

void MainWindow::save()
{
  if (m_filename.empty()) {
    save_as();
  }

  std::ofstream ofs(m_filename);
  if (!ofs) {
    QMessageBox::critical(this, QApplication::applicationDisplayName(),
                          tr("Failed to open '%1' for writing.").arg(QString::fromStdString(m_filename.string())));
  }
  ofs << ::serialize(*m_time_sheet);
}

void MainWindow::save_as()
{
  const auto last_load_dir = QDir::home().path();  // TODO
  const auto q_filename =
      QFileDialog::getSaveFileName(this, QApplication::applicationDisplayName(), last_load_dir, file_filter());

  if (q_filename.isEmpty()) {
    return;
  }
  m_filename = static_cast<std::filesystem::path>(q_filename.toStdString());
  save();
}

void MainWindow::edit_date_time(const QModelIndex& index) const
{
  DateTimeEditor e;
  auto& model = m_time_sheet->interval_model();
  const auto old_date_time = model.data(index, Qt::EditRole).toDateTime();
  e.set_date(old_date_time.date());
  e.set_time(old_date_time.time());
  if (e.exec() == QDialog::Accepted) {
    model.setData(index, e.date_time(), Qt::EditRole);
  }
}

void MainWindow::edit_project(const QModelIndex& index) const
{
  ProjectEditor e(m_time_sheet->project_model());
  auto* const interval = m_time_sheet->interval_model().intervals().at(index.row());
  e.set_project(interval->project());
  if (e.exec() == QDialog::Accepted) {
    if (const auto* const project = e.current_project(); project != nullptr) {
      interval->set_project(*project);
      // TODO emit data changed?
    }
    spdlog::error("Failed to create project.");
  }
}
