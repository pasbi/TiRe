#include "mainwindow.h"
#include "application.h"
#include "commands/addremovecommand.h"
#include "commands/commands.h"
#include "commands/modifycommand.h"
#include "commands/undostack.h"
#include "datetimeeditor.h"
#include "exceptions.h"
#include "intervalmodel.h"
#include "plan.h"
#include "projectmodel.h"
#include "serialization.h"
#include "timesheet.h"
#include "ui_mainwindow.h"

#include <QCloseEvent>
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
  m_ui->period_detail_view->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_ui->period_detail_view, &PeriodDetailView::current_interval_changed, m_ui->ganttview,
          &GanttView::set_current_interval);
  connect(m_ui->period_detail_view, &PeriodDetailView::period_changed, m_ui->ganttview, &GanttView::ensure_visible);
  connect(m_ui->ganttview, &GanttView::clicked, this,
          [this](const QDateTime& timestamp) { set_period(Period{timestamp.date(), Period::Type::Day}); });
  connect(m_ui->action_Load, &QAction::triggered, this, QOverload<>::of(&MainWindow::load));
  connect(m_ui->action_Save, &QAction::triggered, this, &MainWindow::save);
  connect(m_ui->action_Save_As, &QAction::triggered, this, &MainWindow::save_as);
  connect(m_ui->action_New_time_sheet, &QAction::triggered, this, &MainWindow::new_time_sheet);

  connect(m_ui->action_Add_Interval, &QAction::triggered, this, [this]() {
    auto interval = std::make_unique<Interval>(nullptr);
    const auto timestamp = m_current_period.clamp(Application::current_date_time());
    fmt::print("Timestamp: {}, period: {}", timestamp, m_current_period);
    interval->swap_begin(timestamp);
    Application::undo_stack().push(make<AddCommand>(m_time_sheet->interval_model(), std::move(interval)));
  });
  connect(m_ui->action_Switch_Task, &QAction::triggered, this, &MainWindow::switch_task);
  connect(m_ui->actionEnd_Task, &QAction::triggered, this, &MainWindow::end_task);

  connect(m_ui->actionAdd_Plan_Entry, &QAction::triggered, this, [this]() {
    m_ui->tabWidget->setCurrentWidget(m_ui->tv_plan->parentWidget());
    const Period today{Application::current_date_time().date(), Period::Type::Day};
    Application::undo_stack().push(make<AddCommand>(m_time_sheet->plan(), std::make_unique<Plan::Entry>(Plan::Entry{
                                                                              .period = today,
                                                                              .kind = Plan::Kind::Normal,
                                                                          })));
  });

  const auto init_view_action = [this](QAction* action, const Period::Type type) {
    m_view_action_group.addAction(action);
    connect(action, &QAction::triggered, this, [type, this]() { set_period_type(type); });
    connect(this, &MainWindow::period_changed, this, [type, action](const Period& period) {
      if (type == period.type()) {
        action->setChecked(true);
      }
    });
  };
  init_view_action(m_ui->actionYear, Period::Type::Year);
  init_view_action(m_ui->actionMonth, Period::Type::Month);
  init_view_action(m_ui->actionWeek, Period::Type::Week);
  init_view_action(m_ui->actionDay, Period::Type::Day);
  m_ui->actionDay->trigger();

  connect(m_ui->actionNext, &QAction::triggered, this, &MainWindow::next);
  connect(m_ui->actionPrevious, &QAction::triggered, this, &MainWindow::previous);
  connect(m_ui->actionToday, &QAction::triggered, this, &MainWindow::today);

  auto* const undo_action = Application::undo_stack().impl().createUndoAction(this);
  m_ui->menu_Edit->addAction(undo_action);
  undo_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z));
  auto* const redo_action = Application::undo_stack().impl().createRedoAction(this);
  redo_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Y));
  m_ui->menu_Edit->addAction(redo_action);

  connect(&Application::undo_stack().impl(), &QUndoStack::cleanChanged, this,
          [this](const bool clean) { setWindowModified(!clean); });

  new_time_sheet();
}

MainWindow::~MainWindow() = default;

void MainWindow::set_time_sheet(std::unique_ptr<TimeSheet> time_sheet)
{
  m_time_sheet = std::move(time_sheet);
  m_ui->period_detail_view->set_model(m_time_sheet.get());
  m_ui->plan_view->set_model(m_time_sheet.get());
  m_ui->period_summary_view->set_model(m_time_sheet.get());
  m_ui->ganttview->set_time_sheet(m_time_sheet.get());
  m_ui->tv_plan->setModel(&m_time_sheet->plan());
  connect(&m_time_sheet->plan(), &Plan::plan_changed, m_ui->plan_view, &PlanView::invalidate);
  Application::undo_stack().impl().clear();
}

void MainWindow::set_filename(std::filesystem::path filename)
{
  m_filename = std::move(filename);
  update_window_title();
}

void MainWindow::end_task()
{
  auto& interval_model = m_time_sheet->interval_model();
  const auto open_intervals = interval_model.open_intervals();
  if (const auto n = open_intervals.size(); n != 1) {
    QMessageBox::warning(
        this, QApplication::applicationDisplayName(),
        tr("This function can only be called if there is exactly one open interval. Currently open intervals: %1")
            .arg(n),
        QMessageBox::Ok);
    return;
  }
  Application::undo_stack().push(make_modify_interval_command(interval_model, *open_intervals.front(),
                                                              Application::current_date_time(), &Interval::swap_end));
}

void MainWindow::switch_task()
{
  auto& interval_model = m_time_sheet->interval_model();
  const auto open_intervals = interval_model.open_intervals();
  if (const auto n = open_intervals.size(); n > 1) {
    QMessageBox::warning(
        this, QApplication::applicationDisplayName(),
        tr("This function can only be called if there is at most one open interval. Currently open intervals: %1")
            .arg(n),
        QMessageBox::Ok);
    return;
  }

  const auto timestamp = Application::current_date_time();
  auto new_interval = std::make_unique<Interval>(nullptr);
  new_interval->swap_begin(timestamp);
  auto add_interval_command = make<AddCommand>(interval_model, std::move(new_interval));
  const auto macro = Application::undo_stack().start_macro(add_interval_command->text());
  if (!open_intervals.empty()) {
    Application::undo_stack().push(
        make_modify_interval_command(interval_model, *open_intervals.front(), timestamp, &Interval::swap_end));
  }
  Application::undo_stack().push(std::move(add_interval_command));
}

void MainWindow::update_window_title()
{
  const auto filename_part = m_filename.empty() ? tr("Untitled") : QString::fromStdString(m_filename.string());
  static const auto app_now_hint = [] {
    if (const auto app_now = Application::current_date_time(); app_now != QDateTime::currentDateTime()) {
      spdlog::warn("Application now ({}) doesn't match system now ({}). This may be useful for debugging only.",
                   app_now, QDateTime::currentDateTime());
      return tr("TODAY=%1").arg(app_now.toString());
    }
    return QStringLiteral();
  }();
  QStringList title{tr("%1[*] — %2").arg(filename_part, QApplication::applicationDisplayName())};
  if (!app_now_hint.isEmpty()) {
    title.append(app_now_hint);
  }
  setWindowTitle(title.join(" "));
}

bool MainWindow::can_close()
{
  if (Application::undo_stack().impl().isClean()) {
    return true;
  }

  const auto answer = QMessageBox::question(this, QApplication::applicationDisplayName(),
                                            tr("Do you want to save pending changes before close?"),
                                            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Abort);
  return answer == QMessageBox::Discard || (answer == QMessageBox::Save && save());
}

bool MainWindow::load()
{
  const auto last_load_dir = QDir::home().path();  // TODO
  const auto q_filename =
      QFileDialog::getOpenFileName(this, QApplication::applicationDisplayName(), last_load_dir, file_filter());
  if (q_filename.isEmpty()) {
    return false;
  }

  return load(static_cast<std::filesystem::path>(q_filename.toStdString()));
}

bool MainWindow::load(std::filesystem::path filename)
{
  if (!can_close()) {
    return false;
  }

  try {
    std::ifstream ifs(filename);
    if (!ifs) {
      QMessageBox::critical(this, QApplication::applicationDisplayName(),
                            tr("Failed to open '%1' for reading.").arg(QString::fromStdString(m_filename.string())));
    }
    nlohmann::json data;
    ifs >> data;
    set_time_sheet(::deserialize(data));
    set_filename(std::move(filename));
    return true;
  } catch (const DeserializationError& e) {
    QMessageBox::critical(
        this, QApplication::applicationDisplayName(),
        tr("Failed to open '%1': %2").arg(QString::fromStdString(filename.string()), QString::fromStdString(e.what())));
  } catch (const nlohmann::json::parse_error& e) {
    QMessageBox::critical(
        this, QApplication::applicationDisplayName(),
        tr("Failed to open '%1': %2").arg(QString::fromStdString(filename.string()), QString::fromStdString(e.what())));
  }
  return false;
}

bool MainWindow::save()
{
  if (m_filename.empty()) {
    return save_as();
  }

  std::ofstream ofs(m_filename);
  if (!ofs) {
    QMessageBox::critical(this, QApplication::applicationDisplayName(),
                          tr("Failed to open '%1' for writing.").arg(QString::fromStdString(m_filename.string())));
    return false;
  }
  ofs << ::serialize(*m_time_sheet);
  Application::undo_stack().impl().setClean();
  return true;
}

bool MainWindow::save_as()
{
  const auto last_load_dir = QDir::home().path();  // TODO
  const auto q_filename =
      QFileDialog::getSaveFileName(this, QApplication::applicationDisplayName(), last_load_dir, file_filter());

  if (q_filename.isEmpty()) {
    return false;
  }
  set_filename(static_cast<std::filesystem::path>(q_filename.toStdString()));
  save();
  Application::undo_stack().impl().setClean();
  return true;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  can_close() ? event->accept() : event->ignore();
}

bool MainWindow::new_time_sheet()
{
  if (!can_close()) {
    return false;
  }
  set_time_sheet(std::make_unique<TimeSheet>());
  set_filename({});
  set_date(Application::current_date_time().date());
  return true;
}

void MainWindow::next()
{
  set_date(m_current_period.end().addDays(1));
}

void MainWindow::previous()
{
  set_date(m_current_period.begin().addDays(-1));
}

void MainWindow::today()
{
  set_date(Application::current_date_time().date());
}

void MainWindow::set_date(const QDate& date)
{
  set_period(Period(date, m_current_period.type())
                 .constrained(m_time_sheet->plan().start(), Application::current_date_time().date()));
}

void MainWindow::set_period_type(const Period::Type type)
{
  set_period(Period(m_current_period.begin(), type));
}

void MainWindow::set_period(const Period& period)
{
  if (m_current_period == period) {
    return;
  }

  m_current_period = period.constrained(m_time_sheet->plan().start(), Application::current_date_time().date());

  m_ui->period_detail_view->set_period(m_current_period);
  m_ui->plan_view->set_period(m_current_period);
  m_ui->period_summary_view->set_period(m_current_period);
  m_ui->ganttview->select_period(m_current_period);
  m_ui->statusbar->showMessage(m_current_period.label());
  Q_EMIT period_changed(m_current_period);
}
