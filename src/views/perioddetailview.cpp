#include "views/perioddetailview.h"

#include "application.h"
#include "callbackdelegate.h"
#include "commands/commands.h"
#include "commands/undostack.h"
#include "intervalmodel.h"
#include "projecteditor.h"
#include "tableview.h"
#include "timerangeeditor2.h"
#include "timesheet.h"
#include "views/perioddetailproxymodel.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QPainter>
#include <QStyledItemDelegate>
#include <spdlog/spdlog.h>

namespace
{

void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index, Period::Type period_type)
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
  if (option->state & QStyle::State_Selected) {
    option->font.setBold(true);
    option->font.setUnderline(true);
    option->palette.setBrush(QPalette::Highlight, index.data(Qt::BackgroundRole).value<QBrush>());
    option->palette.setBrush(QPalette::HighlightedText, index.data(Qt::ForegroundRole).value<QBrush>());
  }
}

class CallbackItemDelegate : public CallbackDelegate
{
public:
  using CallbackDelegate::CallbackDelegate;
  Period::Type period_type = Period::Type::Custom;

protected:
  void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override
  {
    CallbackDelegate::initStyleOption(option, index);
    ::initStyleOption(option, index, period_type);
  }
};

constexpr auto noop = [](auto&&...) {};

}  // namespace

PeriodDetailView::PeriodDetailView(QWidget* const parent)
  : AbstractPeriodView(parent)
  , m_table_view(::setup_ui_with_single_table_view(this))
  , m_proxy_model(std::make_unique<PeriodDetailProxyModel>())
  , m_ro_item_delegate(std::make_unique<CallbackItemDelegate>(noop, nullptr))
  , m_begin_end_delegate(std::make_unique<CallbackItemDelegate>(
        [this](const QModelIndex& index) { edit_date_time(m_proxy_model->mapToSource(index)); }))
  , m_project_delegate(std::make_unique<CallbackItemDelegate>(
        [this](const QModelIndex& index) { edit_project(m_proxy_model->mapToSource(index)); }))
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
  AbstractPeriodView::set_model(time_sheet);
}

void PeriodDetailView::set_period(const Period& period)
{
  m_proxy_model->set_period(period);
  for (auto& delegate : {m_begin_end_delegate.get(), m_project_delegate.get(), m_ro_item_delegate.get()}) {
    dynamic_cast<CallbackItemDelegate&>(*delegate).period_type = period.type();
  }
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
             [this]() { delete_intervals(time_sheet()->interval_model(), selected_intervals()); });
  add_action(tr("Split"), Qt::CTRL | Qt::Key_Comma, [this]() {
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
  TimeRangeEditor2 e;
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

void PeriodDetailView::edit_project(const QModelIndex& index) const
{
  ProjectEditor e(time_sheet()->project_model());
  auto* const interval = time_sheet()->interval_model().intervals().at(index.row());
  if (const auto* const project = interval->project(); project != nullptr) {
    e.set_project(*project);
  }
  if (e.exec() == QDialog::Accepted) {
    Application::undo_stack().push(make_modify_interval_command(time_sheet()->interval_model(), *interval,
                                                                &e.current_project(), &Interval::swap_project));
  }
}
