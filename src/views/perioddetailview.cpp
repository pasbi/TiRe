#include "views/perioddetailview.h"

#include "application.h"
#include "callbackdelegate.h"
#include "commands/addremovecommand.h"
#include "commands/commands.h"
#include "commands/undostack.h"
#include "enumcombobox.h"
#include "intervalmodel.h"
#include "projectmodel.h"
#include "tableview.h"
#include "timerangeeditor.h"
#include "timesheet.h"
#include "views/perioddetailproxymodel.h"

#include <QApplication>
#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QStyledItemDelegate>
#include <spdlog/spdlog.h>

namespace
{

void init_style_selected(QStyleOptionViewItem* option, const QModelIndex& index)
{
  if (option->state & QStyle::State_Selected) {
    option->font.setBold(true);
    option->font.setUnderline(true);
    option->palette.setBrush(QPalette::Highlight, index.data(Qt::BackgroundRole).value<QBrush>());
    option->palette.setBrush(QPalette::HighlightedText, index.data(Qt::ForegroundRole).value<QBrush>());
  }
}

void init_style_option(QStyleOptionViewItem* option, const QModelIndex& index, const Period::Type period_type)
{
  if (const auto data = index.data(Qt::DisplayRole); data.typeId() == qMetaTypeId<DatePair>()) {
    const auto& [begin, end] = qvariant_cast<DatePair>(data);
    using enum Period::Type;
    const auto format = period_type == Custom || period_type == Year ? "dd.MM." : "ddd, dd.";
    if (begin == end) {
      option->text = begin.toString(format);
    } else {
      option->text = begin.toString(format) + " - " + end.toString(format);
    }
  }
  ::init_style_selected(option, index);
}

class ROItemDelegate final : public QStyledItemDelegate
{
protected:
  void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override
  {
    QStyledItemDelegate::initStyleOption(option, index);
    ::init_style_selected(option, index);
  }
};

class CallbackItemDelegate final : public CallbackDelegate
{
public:
  using CallbackDelegate::CallbackDelegate;
  Period::Type period_type = Period::Type::Custom;

protected:
  void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override
  {
    CallbackDelegate::initStyleOption(option, index);
    ::init_style_option(option, index, period_type);
  }
};

class ProjectItemDelegate final : public QStyledItemDelegate
{
public:
  explicit ProjectItemDelegate(PeriodDetailProxyModel& proxy_model, QObject* parent = nullptr)
    : QStyledItemDelegate(parent), m_proxy_model(proxy_model)
  {
  }

  QWidget* createEditor(QWidget* const parent, const QStyleOptionViewItem&, const QModelIndex& index) const override
  {
    auto editor = std::make_unique<QComboBox>(parent);
    editor->setEditable(true);
    for (const auto& project : m_time_sheet->project_model().projects()) {
      editor->addItem(project->name());
    }
    editor->addItem(tr("No Project"));
    return editor.release();
  }

  void setEditorData(QWidget* const editor, const QModelIndex& index) const override
  {
    auto& combo_box = dynamic_cast<QComboBox&>(*editor);
    if (const auto* const project = interval(index).project(); project != nullptr) {
      combo_box.setCurrentText(project->name());
    }
    combo_box.lineEdit()->selectAll();
  }

  void setModelData(QWidget* const editor, QAbstractItemModel* const model, const QModelIndex& index) const override
  {
    const auto& combo_box = dynamic_cast<QComboBox&>(*editor);
    const Project* project = nullptr;
    if (combo_box.currentIndex() == m_time_sheet->project_model().projects().size()) {
      project = nullptr;
    } else if (const auto current_text = combo_box.currentText();
               current_text == combo_box.itemText(combo_box.currentIndex()))
    {
      project = &m_time_sheet->project_model().project(combo_box.currentIndex());
    } else if (QMessageBox::question(editor, QApplication::applicationDisplayName(),
                                     tr("There is no project '%1'. Do you want to create it?").arg(current_text),
                                     QMessageBox::Yes | QMessageBox::No)
               == QMessageBox::Yes)
    {
      project = &create_project(current_text);
    } else {
      return;
    }
    Application::undo_stack().push(make_modify_interval_command(m_time_sheet->interval_model(), interval(index),
                                                                project, &Interval::swap_project));
  }

  void set_time_sheet(const TimeSheet* const time_sheet)
  {
    m_time_sheet = time_sheet;
  }

protected:
  void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override
  {
    QStyledItemDelegate::initStyleOption(option, index);
    ::init_style_selected(option, index);
  }

private:
  const TimeSheet* m_time_sheet = nullptr;
  PeriodDetailProxyModel& m_proxy_model;

  [[nodiscard]] const Interval& interval(const QModelIndex& index) const
  {
    return *m_time_sheet->interval_model().interval(m_proxy_model.mapToSource(index).row());
  }

  [[nodiscard]] const Project& create_project(const QString& name) const
  {
    auto& project_model = m_time_sheet->project_model();
    auto new_project = std::make_unique<Project>(name, project_model.generate_color());
    auto& ref = *new_project;
    Application::undo_stack().push(::make<AddCommand>(project_model, std::move(new_project)));
    return ref;
  }
};

}  // namespace

PeriodDetailView::PeriodDetailView(QWidget* const parent)
  : AbstractPeriodView(parent)
  , m_table_view(::setup_ui_with_single_table_view(this))
  , m_proxy_model(std::make_unique<PeriodDetailProxyModel>())
  , m_ro_item_delegate(std::make_unique<ROItemDelegate>())
  , m_begin_end_delegate(std::make_unique<CallbackItemDelegate>(
        [this](const QModelIndex& index) { edit_date_time(m_proxy_model->mapToSource(index)); }))
  , m_project_delegate(std::make_unique<ProjectItemDelegate>(*m_proxy_model))
{
  init_context_menu_actions();
  m_table_view.setModel(m_proxy_model.get());
  m_table_view.setSelectionBehavior(QAbstractItemView::SelectRows);
  m_table_view.setSelectionMode(QAbstractItemView::SingleSelection);
  m_table_view.setItemDelegateForColumn(0, m_project_delegate.get());
  m_table_view.setItemDelegateForColumn(1, m_begin_end_delegate.get());
  m_table_view.setItemDelegateForColumn(2, m_begin_end_delegate.get());
  m_table_view.setItemDelegateForColumn(3, m_begin_end_delegate.get());
  m_table_view.setItemDelegateForColumn(4, m_ro_item_delegate.get());
  m_table_view.setSortingEnabled(true);
  m_table_view.sortByColumn(IntervalModel::begin_column, Qt::AscendingOrder);
  connect(m_table_view.selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex& index) {
    const auto source_index = m_proxy_model->mapToSource(index);
    if (!source_index.isValid()) {
      Q_EMIT current_interval_changed(nullptr);
      return;
    }
    const auto* const interval = time_sheet()->interval_model().interval(source_index.row());
    Q_EMIT current_interval_changed(interval);
  });
  connect(this, &QWidget::customContextMenuRequested, this,
          [this](const QPoint& pos) { show_table_context_menu(mapToGlobal(pos)); });
}

PeriodDetailView::~PeriodDetailView() = default;

const Interval* PeriodDetailView::current_interval() const
{
  if (const auto index = m_table_view.currentIndex(); index.isValid()) {
    const auto row = m_proxy_model->mapToSource(index).row();
    return time_sheet()->interval_model().interval(row);
  }
  return nullptr;
}

std::set<const Interval*> PeriodDetailView::selected_intervals() const
{
  std::set<const Interval*> selection;
  for (const auto& index : m_table_view.selectionModel()->selectedRows()) {
    const auto row = m_proxy_model->mapToSource(index).row();
    selection.insert(time_sheet()->interval_model().interval(row));
  }
  return selection;
}

void PeriodDetailView::set_model(const TimeSheet* time_sheet)
{
  m_proxy_model->set_source_model(time_sheet == nullptr ? nullptr : &time_sheet->interval_model());
  dynamic_cast<ProjectItemDelegate&>(*m_project_delegate).set_time_sheet(time_sheet);
  AbstractPeriodView::set_model(time_sheet);
}

void PeriodDetailView::set_period(const Period& period)
{
  m_proxy_model->set_period(period);
  dynamic_cast<CallbackItemDelegate&>(*m_begin_end_delegate).period_type = period.type();
  AbstractPeriodView::set_period(period);
}

void PeriodDetailView::invalidate()
{
  update();
}

void PeriodDetailView::init_context_menu_actions()
{
  const auto add_action = [this](const QString& label, const QKeySequence& shortcut, auto slot) {
    auto& action = *m_context_menu_actions.emplace_back(std::make_unique<QAction>(label));
    addAction(&action);
    action.setShortcut(shortcut);
    connect(&action, &QAction::triggered, this, slot);
  };
  add_action(tr("Delete"), QKeySequence(Qt::Key_Delete),
             [this] { delete_intervals(time_sheet()->interval_model(), selected_intervals()); });
  add_action(tr("Split"), Qt::CTRL | Qt::Key_Comma, [this] {
    if (const auto* const interval = current_interval(); interval != nullptr) {
      split_interval(time_sheet()->interval_model(), *interval);
    }
  });
}

void PeriodDetailView::show_table_context_menu(const QPoint& pos)
{
  QMenu menu;
  for (const auto& action : m_context_menu_actions) {
    menu.addAction(action.get());
  }
  menu.exec(pos);
}

void PeriodDetailView::edit_date_time(const QModelIndex& index) const
{
  TimeRangeEditor e;
  auto& interval = *time_sheet()->interval_model().intervals().at(index.row());
  e.set_range(interval.begin(), interval.end());
  if (e.exec() == QDialog::Accepted) {
    const auto macro = Application::undo_stack().start_macro(tr("Change interval"));
    Application::undo_stack().push(
        make_modify_interval_command(time_sheet()->interval_model(), interval, e.begin(), &Interval::swap_begin));
    Application::undo_stack().push(
        make_modify_interval_command(time_sheet()->interval_model(), interval, e.end(), &Interval::swap_end));
    Q_EMIT time_sheet()->interval_model().data_changed();
  }
}
