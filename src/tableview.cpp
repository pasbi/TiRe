#include "tableview.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <cstdint>
#include <numeric>
#include <spdlog/spdlog.h>

namespace
{

/**
 * @brief Give every bin the size its content asks for, and share whatever `total_size` has left
 *  over in proportion to that -- so the column with the most to show, the project names usually,
 *  keeps the largest part of the slack rather than an equal fifth of it.
 *
 * Content that does not fit at all is not squeezed: the bins keep their full size hint and the
 * view scrolls, which beats cutting every column short.
 */
[[nodiscard]] std::vector<int> distribute(const int total_size, const std::vector<int>& size_hints)
{
  const auto required = std::accumulate(size_hints.begin(), size_hints.end(), 0);
  if (required >= total_size || required == 0) {
    return size_hints;
  }

  const auto surplus = total_size - required;
  // Hand out the surplus against the running total rather than bin by bin, so the rounding losses
  // cannot pile up into a gap at the right edge.
  auto accumulated_hint = 0;
  auto distributed = 0;
  std::vector<int> sizes;
  sizes.reserve(size_hints.size());
  for (const auto& size_hint : size_hints) {
    accumulated_hint += size_hint;
    const auto share = static_cast<int>(static_cast<std::int64_t>(surplus) * accumulated_hint / required) - distributed;
    distributed += share;
    sizes.emplace_back(size_hint + share);
  }
  return sizes;
}

}  // namespace

TableView::TableView(QWidget* parent) : QTableView(parent)
{
  horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
}

void TableView::setModel(QAbstractItemModel* model)
{
  QTableView::setModel(model);
  if (model == nullptr) {
    return;
  }
  // Whatever changes the cells can change what the columns need, so every such signal recomputes.
  // Queued, because the hints come from the rows the view has laid out: it must see the change
  // first.
  const auto recompute = [this] { update_column_widths(); };
  connect(model, &QAbstractItemModel::modelReset, this, recompute, Qt::QueuedConnection);
  connect(model, &QAbstractItemModel::layoutChanged, this, recompute, Qt::QueuedConnection);
  connect(model, &QAbstractItemModel::dataChanged, this, recompute, Qt::QueuedConnection);
  connect(model, &QAbstractItemModel::rowsInserted, this, recompute, Qt::QueuedConnection);
  connect(model, &QAbstractItemModel::rowsRemoved, this, recompute, Qt::QueuedConnection);
  connect(model, &QAbstractItemModel::columnsInserted, this, recompute, Qt::QueuedConnection);
  connect(model, &QAbstractItemModel::columnsRemoved, this, recompute, Qt::QueuedConnection);
}

void TableView::update_column_widths()
{
  if (model() == nullptr) {
    return;
  }
  QAbstractScrollArea::updateGeometry();
  const auto column_count = model()->columnCount();
  std::vector<int> size_hints;
  size_hints.reserve(column_count);
  for (int i = 0; i < column_count; ++i) {
    size_hints.emplace_back(sizeHintForColumn(i));
  }

  const auto sizes = ::distribute(maximumViewportSize().width(), size_hints);
  for (int i = 0; i < static_cast<int>(sizes.size()); ++i) {
    setColumnWidth(i, sizes.at(i));
  }
}

void TableView::resizeEvent(QResizeEvent* event)
{
  update_column_widths();
  QTableView::resizeEvent(event);
}

TableView& setup_ui_with_single_table_view(QWidget* const container)
{
  container->setLayout(new QHBoxLayout);
  container->layout()->setContentsMargins(0, 0, 0, 0);
  auto table_view = std::make_unique<TableView>();
  auto& ref = *table_view;
  container->layout()->addWidget(table_view.release());
  return ref;
}
