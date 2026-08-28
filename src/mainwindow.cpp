#include "mainwindow.h"
#include "application.h"
#include "commands/addremovecommand.h"
#include "commands/commands.h"
#include "commands/modifycommand.h"
#include "commands/undostack.h"
#include "exceptions.h"
#include "intervalmodel.h"
#include "plan.h"
#include "plansettingsdialog.h"
#include "projectmodel.h"
#include "timesheet.h"
#include "ui_mainwindow.h"

#include <QCloseEvent>
#include <QLabel>
#include <QMessageBox>
#include <QStatusBar>
#include <fmt/chrono.h>
#include <spdlog/spdlog.h>

namespace
{

// Pane order inside the main splitter, as laid out in mainwindow.ui.
constexpr auto plan_view_pane = 0;
constexpr auto period_detail_pane = 1;
constexpr auto tab_pane = 2;

// How the space left over after the plan view is shared between the interval table and the tabs.
constexpr auto period_detail_share = 4;
constexpr auto tab_share = 3;

}  // namespace

MainWindow::MainWindow(std::unique_ptr<TimeSheet> time_sheet)
  : m_ui(std::make_unique<Ui::MainWindow>()), m_view_action_group(this)
{
  m_ui->setupUi(this);
  set_time_sheet(std::move(time_sheet));

  m_persistence_status_label = new QLabel{this};
  m_persistence_status_label->setStyleSheet(QStringLiteral("QLabel { color: red; }"));
  m_persistence_status_label->hide();
  // A permanent widget, not showMessage(): the status bar's transient slot is used for the
  // period label and would overwrite a warning on the next navigation keystroke.
  statusBar()->addPermanentWidget(m_persistence_status_label);
  connect(&Application::undo_stack(), &UndoStack::write_failed, this, &MainWindow::on_write_failed);
  connect(&Application::undo_stack(), &UndoStack::write_succeeded, this, &MainWindow::on_write_succeeded);

  // One splitter across all three panes, so every boundary is draggable and there is no nesting.
  //
  // The plan view is a fixed set of summary labels: it has a natural width and gains nothing from
  // being wider, so it takes no share of spare space and the other two divide it between them.
  //
  // Stretch factors alone only govern how *surplus* is shared when the window is resized -- the
  // initial split is proportional to the size hints, which left the plan view far wider than it
  // needs. Asking for its size hint and oversized values for the rest pins it: the splitter
  // clamps those down to the space actually left, in the ratio given.
  m_ui->splitter->setStretchFactor(plan_view_pane, 0);
  m_ui->splitter->setStretchFactor(period_detail_pane, period_detail_share);
  m_ui->splitter->setStretchFactor(tab_pane, tab_share);
  // The summary and the table are the primary views and must not vanish by accident; the tab pane
  // may be dragged shut, as it could be before.
  m_ui->splitter->setCollapsible(plan_view_pane, false);
  m_ui->splitter->setCollapsible(period_detail_pane, false);
  m_ui->splitter->setCollapsible(tab_pane, true);
  static constexpr auto total_share = period_detail_share + tab_share;
  m_ui->splitter->setSizes({m_ui->plan_view->sizeHint().width(), period_detail_share * QWIDGETSIZE_MAX / total_share,
                            tab_share * QWIDGETSIZE_MAX / total_share});

  m_ui->period_detail_view->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_ui->period_detail_view, &PeriodDetailView::current_interval_changed, m_ui->ganttview,
          &GanttView::set_current_interval);
  connect(m_ui->period_detail_view, &PeriodDetailView::period_changed, m_ui->ganttview, &GanttView::ensure_visible);
  connect(m_ui->ganttview, &GanttView::clicked, this,
          [this](const QDateTime& timestamp) { set_period(Period{timestamp.date(), Period::Type::Day}); });
  connect(m_ui->actionQuit, &QAction::triggered, this, &QMainWindow::close);
  connect(m_ui->actionPlan_Settings, &QAction::triggered, this, &MainWindow::edit_plan_settings);

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
    Application::undo_stack().push(
        make<AddCommand>(m_time_sheet->plan(), std::make_unique<Plan::Entry>(Plan::Entry{
                                                   .period = m_time_sheet->plan().default_period(),
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

  connect(m_ui->actionNext, &QAction::triggered, this, &MainWindow::next);
  connect(m_ui->actionPrevious, &QAction::triggered, this, &MainWindow::previous);
  connect(m_ui->actionToday, &QAction::triggered, this, &MainWindow::today);

  // Built by hand rather than with QUndoStack::createUndoAction, because those trigger
  // QUndoStack::undo()/redo() directly and would bypass UndoStack's transaction handling --
  // undoing a forty-interval macro would then run forty separate transactions.
  auto* const undo_action = new QAction{tr("&Undo"), this};
  undo_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z));
  undo_action->setEnabled(Application::undo_stack().impl().canUndo());
  connect(undo_action, &QAction::triggered, this, [] { Application::undo_stack().undo(); });
  connect(&Application::undo_stack().impl(), &QUndoStack::canUndoChanged, undo_action, &QAction::setEnabled);
  m_ui->menu_Edit->addAction(undo_action);

  auto* const redo_action = new QAction{tr("&Redo"), this};
  redo_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Y));
  redo_action->setEnabled(Application::undo_stack().impl().canRedo());
  connect(redo_action, &QAction::triggered, this, [] { Application::undo_stack().redo(); });
  connect(&Application::undo_stack().impl(), &QUndoStack::canRedoChanged, redo_action, &QAction::setEnabled);
  m_ui->menu_Edit->addAction(redo_action);

  update_window_title();
  // Establishing the period must come last: set_period reads Plan::start(), so it needs the
  // loaded plan, and it pushes the period into views that must already have their models.
  m_ui->actionDay->trigger();
  today();
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
  static const auto app_now_hint = [] {
    if (const auto app_now = Application::current_date_time(); app_now != QDateTime::currentDateTime()) {
      spdlog::warn("Application now ({}) doesn't match system now ({}). This may be useful for debugging only.",
                   app_now, QDateTime::currentDateTime());
      return tr("TODAY=%1").arg(app_now.toString());
    }
    return QStringLiteral();
  }();
  QStringList title{QApplication::applicationDisplayName()};
  if (!app_now_hint.isEmpty()) {
    title.append(app_now_hint);
  }
  // The default database needs no mention; an override does, so it is never unclear which records
  // are being edited.
  if (!Application::is_default_database()) {
    title.append(tr("DB=%1").arg(QString::fromStdString(Application::database_path().string())));
  }
  setWindowTitle(title.join(" "));
}

void MainWindow::on_write_failed(const QString& message)
{
  m_persistence_status_label->setText(tr("Not saving — changes are only in memory"));
  m_persistence_status_label->show();
  if (m_persistence_failed) {
    // Already reported. A broken database fails on every keystroke, and one modal per failure
    // would be an unclosable dialog storm.
    return;
  }
  m_persistence_failed = true;
  QMessageBox::critical(this, QApplication::applicationDisplayName(),
                        tr("Cannot write to the database '%1':\n\n%2\n\nYour recent changes are still shown but "
                           "are not saved. Copy anything you cannot lose before closing.")
                            .arg(QString::fromStdString(Application::database_path().string()), message));
}

void MainWindow::on_write_succeeded()
{
  if (!m_persistence_failed) {
    return;
  }
  m_persistence_failed = false;
  m_persistence_status_label->hide();
}

void MainWindow::edit_plan_settings()
{
  PlanSettingsDialog dialog{m_time_sheet->plan(), this};
  dialog.exec();
}

bool MainWindow::can_close()
{
  if (!m_persistence_failed) {
    return true;
  }
  const auto answer =
      QMessageBox::warning(this, QApplication::applicationDisplayName(),
                           tr("Recent changes could not be written to the database and will be lost. Quit anyway?"),
                           QMessageBox::Discard | QMessageBox::Cancel);
  return answer == QMessageBox::Discard;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  can_close() ? event->accept() : event->ignore();
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
