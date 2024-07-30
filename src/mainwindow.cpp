#include "mainwindow.h"
#include "commands/addremoveintervalcommand.h"
#include "commands/modifycommand.h"
#include "commands/undostack.h"
#include "datetimeeditor.h"
#include "exceptions.h"
#include "intervalmodel.h"
#include "plan.h"
#include "projecteditor.h"
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

template<typename IntervalT, typename Value, typename Swapper> std::unique_ptr<Command>
make_modify_interval_command(IntervalModel& interval_model, IntervalT& interval, Value other_value, Swapper swapper)
{
  if constexpr (std::is_const_v<IntervalT>) {
    return make_modify_interval_command(interval_model, interval_model.remove_const(interval), std::move(other_value),
                                        std::move(swapper));
  } else {
    const auto signal = [&interval_model, &interval]() {
      const auto index = interval_model.index(interval);
      Q_EMIT interval_model.dataChanged(index, index);
      Q_EMIT interval_model.data_changed();
    };
    return make_modify_command(interval, std::move(other_value), std::move(swapper), std::move(signal));
  }
}

}  // namespace

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
  , m_ui(std::make_unique<Ui::MainWindow>())
  , m_time_sheet(std::make_unique<TimeSheet>())
  , m_view_action_group(this)
  , m_undo_stack(std::make_unique<UndoStack>())
{
  m_ui->setupUi(this);
  connect(m_ui->period_summary, &PeriodSummary::double_clicked, this, [this](const QModelIndex& index) {
    if (index.column() == IntervalModel::begin_column || index.column() == IntervalModel::end_column) {
      edit_date_time(index);
    } else if (index.column() == IntervalModel::project_column) {
      edit_project(index);
    }
  });
  m_ui->period_summary->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_ui->period_summary, &QWidget::customContextMenuRequested, this,
          [this](const QPoint& pos) { show_table_context_menu(m_ui->period_summary->mapToGlobal(pos)); });
  connect(m_ui->action_Load, &QAction::triggered, this, QOverload<>::of(&MainWindow::load));
  connect(m_ui->action_Save, &QAction::triggered, this, &MainWindow::save);
  connect(m_ui->action_Save_As, &QAction::triggered, this, &MainWindow::save_as);
  connect(m_ui->action_New_time_sheet, &QAction::triggered, this, &MainWindow::new_time_sheet);

  connect(m_ui->action_Add_Interval, &QAction::triggered, this, [this]() {
    auto interval = std::make_unique<Interval>(m_time_sheet->project_model().empty_project());
    interval->swap_begin(QDateTime::currentDateTime());
    m_undo_stack->push(std::make_unique<AddIntervalCommand>(m_time_sheet->interval_model(), std::move(interval)));
  });

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

  auto* const undo_action = m_undo_stack->impl().createUndoAction(this);
  m_ui->menu_Edit->addAction(undo_action);
  undo_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z));
  auto* const redo_action = m_undo_stack->impl().createRedoAction(this);
  redo_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Y));
  m_ui->menu_Edit->addAction(redo_action);

  connect(&m_undo_stack->impl(), &QUndoStack::cleanChanged, this,
          [this](const bool clean) { setWindowModified(!clean); });

  init_context_menu_actions();

  new_time_sheet();
}

MainWindow::~MainWindow() = default;

void MainWindow::set_time_sheet(std::unique_ptr<TimeSheet> time_sheet)
{
  m_time_sheet = std::move(time_sheet);
  m_ui->period_summary->set_model(m_time_sheet->interval_model(), m_time_sheet->plan());
  m_undo_stack->impl().clear();
}

void MainWindow::set_filename(std::filesystem::path filename)
{
  m_filename = std::move(filename);
  update_window_title();
}

void MainWindow::set_period_type(const Period::Type type)
{
  m_ui->period_summary->set_period_type(type);
}

void MainWindow::update_window_title()
{
  const auto filename_part = m_filename.empty() ? tr("Untitled") : QString::fromStdString(m_filename.string());
  setWindowTitle(tr("%1[*] — %2").arg(filename_part, QApplication::applicationDisplayName()));
}

bool MainWindow::can_close()
{
  if (m_undo_stack->impl().isClean()) {
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
  m_undo_stack->impl().setClean();
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
  m_undo_stack->impl().setClean();
  return true;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  can_close() ? event->accept() : event->ignore();
}

void MainWindow::delete_selected_intervals() const
{
  const auto macro = m_undo_stack->start_macro(tr("Delete selected intervals"));
  for (const auto* const interval : m_ui->period_summary->selected_intervals()) {
    m_undo_stack->push(std::make_unique<RemoveIntervalCommand>(m_time_sheet->interval_model(), *interval));
  }
}

void MainWindow::split_selected_intervals() const
{
  const auto* const interval = m_ui->period_summary->current_interval();
  if (interval == nullptr) {
    return;
  }
  DateTimeEditor e;
  e.set_date(interval->begin().date());
  e.set_time(interval->begin().time());
  // TODO
  // e.set_range(interval.begin(), interval.end());
  if (e.exec() == QDialog::Accepted) {
    const auto macro = m_undo_stack->start_macro(tr("Delete selected intervals"));
    auto new_interval = std::make_unique<Interval>(interval->project());
    new_interval->swap_begin(e.date_time());
    new_interval->swap_end(interval->end());
    m_undo_stack->push(std::make_unique<AddIntervalCommand>(m_time_sheet->interval_model(), std::move(new_interval)));
    m_undo_stack->push(
        make_modify_interval_command(m_time_sheet->interval_model(), *interval, e.date_time(), &Interval::swap_end));
  }
}

void MainWindow::init_context_menu_actions()
{
  const auto add_action = [this](const QString& label, const QKeySequence& shortcut, auto slot) {
    auto& action = *m_context_menu_actions.emplace_back(std::make_unique<QAction>(label));
    addAction(&action);
    action.setShortcut(shortcut);
    connect(&action, &QAction::triggered, this, slot);
  };
  add_action(tr("Delete"), QKeySequence(Qt::Key_Delete), &MainWindow::delete_selected_intervals);
  add_action(tr("Split"), Qt::CTRL | Qt::Key_Comma, &MainWindow::split_selected_intervals);
}

void MainWindow::show_table_context_menu(const QPoint& pos)
{
  QMenu menu;
  for (const auto& action : m_context_menu_actions) {
    menu.addAction(action.get());
  }
  menu.exec(pos);
}

void MainWindow::edit_date_time(const QModelIndex& index) const
{
  DateTimeEditor e;
  auto& interval = *m_time_sheet->interval_model().intervals().at(index.row());
  const auto old_date_time = index.column() == IntervalModel::begin_column ? interval.begin() : interval.end();
  e.set_date(old_date_time.date());
  e.set_time(old_date_time.time());
  if (e.exec() == QDialog::Accepted) {
    std::unique_ptr<Command> command;
    if (index.column() == IntervalModel::begin_column) {
      m_undo_stack->push(
          make_modify_interval_command(m_time_sheet->interval_model(), interval, e.date_time(), &Interval::swap_begin));
    } else {
      m_undo_stack->push(
          make_modify_interval_command(m_time_sheet->interval_model(), interval, e.date_time(), &Interval::swap_end));
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
    m_undo_stack->push(make_modify_interval_command(m_time_sheet->interval_model(), *interval, &e.current_project(),
                                                    &Interval::swap_project));
  }
}

bool MainWindow::new_time_sheet()
{
  if (!can_close()) {
    return false;
  }
  set_time_sheet(std::make_unique<TimeSheet>());
  set_filename({});
  m_ui->period_summary->set_date(QDate::currentDate());
  return true;
}
