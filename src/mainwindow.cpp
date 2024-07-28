#include "mainwindow.h"
#include "datetimeeditor.h"
#include "exceptions.h"
#include "intervalmodel.h"
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
  : QMainWindow(parent)
  , m_ui(std::make_unique<Ui::MainWindow>())
  , m_time_sheet(std::make_unique<TimeSheet>())
  , m_view_action_group(this)
{
  m_ui->setupUi(this);
  connect(m_ui->period_summary, &PeriodSummary::double_clicked, this, [this](const QModelIndex& index) {
    if (index.column() == IntervalModel::begin_column || index.column() == IntervalModel::end_column) {
      edit_date_time(index);
    } else if (index.column() == IntervalModel::project_column) {
      edit_project(index);
    }
  });
  connect(m_ui->action_Load, &QAction::triggered, this, QOverload<>::of(&MainWindow::load));
  connect(m_ui->action_Save, &QAction::triggered, this, &MainWindow::save);
  connect(m_ui->action_Save_As, &QAction::triggered, this, &MainWindow::save_as);

  const auto init_view_action = [this](QAction* action, const Period::Type type) {
    m_view_action_group.addAction(action);
    connect(action, &QAction::triggered, this, [type, this]() { set_period_type(type); });
  };
  init_view_action(m_ui->actionYear, Period::Type::Year);
  init_view_action(m_ui->actionMonth, Period::Type::Month);
  init_view_action(m_ui->actionWeek, Period::Type::Week);
  init_view_action(m_ui->actionDay, Period::Type::Day);
  m_ui->actionDay->trigger();

  connect(m_ui->actionNext, &QAction::triggered, m_ui->period_summary, &PeriodSummary::next);
  connect(m_ui->actionPrevious, &QAction::triggered, m_ui->period_summary, &PeriodSummary::prev);
  connect(m_ui->actionToday, &QAction::triggered, m_ui->period_summary, &PeriodSummary::today);

  set_time_sheet(std::make_unique<TimeSheet>());
  m_ui->period_summary->set_date(QDate::currentDate());
}

MainWindow::~MainWindow() = default;

void MainWindow::set_time_sheet(std::unique_ptr<TimeSheet> time_sheet)
{
  m_time_sheet = std::move(time_sheet);
  connect(m_ui->action_Add_Interval, &QAction::triggered, this,
          [this]() { m_time_sheet->interval_model().new_interval(m_time_sheet->project_model().empty_project()); });

  m_ui->period_summary->set_model(m_time_sheet->interval_model());
}

void MainWindow::set_period_type(const Period::Type type)
{
  m_ui->period_summary->set_period_type(type);
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
  auto& interval = *m_time_sheet->interval_model().intervals().at(index.row());
  const auto old_date_time = index.column() == IntervalModel::begin_column ? interval.begin() : interval.end();
  e.set_date(old_date_time.date());
  e.set_time(old_date_time.time());
  if (e.exec() == QDialog::Accepted) {
    if (index.column() == IntervalModel::begin_column) {
      interval.set_begin(e.date_time());
    } else {
      interval.set_end(e.date_time());
    }
    Q_EMIT m_time_sheet->interval_model().data_changed();
  }
}

void MainWindow::edit_project(const QModelIndex& index) const
{
  ProjectEditor e(m_time_sheet->project_model());
  auto* const interval = m_time_sheet->interval_model().intervals().at(index.row());
  e.set_project(interval->project());
  if (e.exec() == QDialog::Accepted) {
    interval->set_project(e.current_project());
    Q_EMIT m_time_sheet->interval_model().data_changed();
  }
}
