#include "views/perioddetailview.h"
#include "commands/commands.h"
#include "intervalmodel.h"
#include "tableview.h"
#include "timesheet.h"
#include "views/perioddetailproxymodel.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QPainter>
#include <QStyledItemDelegate>
#include <spdlog/spdlog.h>

class PeriodDetailView::ItemDelegate : public QStyledItemDelegate
{
public:
  void set_period_type(const Period::Type type)
  {
    m_period_type = type;
  }

protected:
  void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override
  {
    QStyledItemDelegate::initStyleOption(option, index);
    if (const auto data = index.data(Qt::DisplayRole); data.typeId() == qMetaTypeId<DatePair>()) {
      const auto& [begin, end] = qvariant_cast<DatePair>(data);
      using enum Period::Type;
      const auto format = m_period_type == Custom || m_period_type == Year ? "dd.MM." : "ddd, dd.";
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

private:
  Period::Type m_period_type = Period::Type::Custom;
};

PeriodDetailView::PeriodDetailView(QWidget* parent)
  : AbstractPeriodView(parent)
  , m_table_view(::setup_ui_with_single_table_view(this))
  , m_proxy_model(std::make_unique<PeriodDetailProxyModel>())
  , m_item_delegate(std::make_unique<ItemDelegate>())
{
  init_context_menu_actions();
  connect(&m_table_view, &QAbstractItemView::doubleClicked, this,
          [this](const QModelIndex& index) { Q_EMIT double_clicked(m_proxy_model->mapToSource(index)); });
  m_table_view.setModel(m_proxy_model.get());
  m_table_view.setSelectionBehavior(QAbstractItemView::SelectRows);
  m_table_view.setSelectionMode(QAbstractItemView::SingleSelection);
  m_table_view.setItemDelegate(m_item_delegate.get());
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
  m_item_delegate->set_period_type(period.type());
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
