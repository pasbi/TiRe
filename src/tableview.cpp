#include "tableview.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <spdlog/spdlog.h>

namespace
{

/**
* @brief Distribute the `total_size` evenly among the bins, but don't shrink below its size hint.
*/
class SizesCalculator
{
public:
  struct Size
  {
    explicit Size(const int size_hint) : size_hint(size_hint)
    {
    }

    const int size_hint;
    int actual_size;
    bool fixed = false;
  };

  explicit SizesCalculator(const int total_size, const std::vector<int>& size_hints) : m_total_size(total_size)
  {
    m_sizes.reserve(size_hints.size());
    for (const auto& size_hint : size_hints) {
      m_sizes.emplace_back(size_hint);
    }

    while (iterate()) {
    }
  }

  [[nodiscard]] std::vector<int> sizes() const
  {
    std::vector<int> sizes;
    sizes.reserve(sizes.size());
    for (const auto& size : m_sizes) {
      sizes.emplace_back(size.actual_size);
    }
    return sizes;
  }

private:
  [[nodiscard]] int count_unfixed() const
  {
    return static_cast<int>(m_sizes.size() - std::ranges::count_if(m_sizes, &Size::fixed));
  }

  [[nodiscard]] int left_size() const
  {
    static constexpr auto accu = [](const int accu, const Size& size) {
      return size.fixed ? size.actual_size + accu : accu;
    };
    return m_total_size - std::accumulate(m_sizes.begin(), m_sizes.end(), 0, accu);
  }

  bool iterate()
  {
    const auto unfixed_count = count_unfixed();
    if (unfixed_count == 0) {
      return false;  // all sizes are fixed, abort iteration.
    }

    const auto ideal_width = left_size() / unfixed_count;
    bool greater_than_average = false;
    for (auto& size : m_sizes) {
      if (size.fixed) {
        continue;
      }
      if (size.size_hint > ideal_width) {
        size.actual_size = size.size_hint;
        size.fixed = true;
        greater_than_average = true;
      } else {
        size.actual_size = ideal_width;
      }
    }
    // continue iteration if any size requires more space than it would get if the left space was uniformly distrubted.
    return greater_than_average;
  }

  const int m_total_size;
  std::vector<Size> m_sizes;
};

}  // namespace

TableView::TableView(QWidget* parent) : QTableView(parent)
{
  horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
}

void TableView::setModel(QAbstractItemModel* model)
{
  connect(model, &QAbstractItemModel::modelReset, this, &TableView::update_column_widths, Qt::QueuedConnection);
  connect(model, &QAbstractItemModel::columnsAboutToBeInserted, this, &TableView::update_column_widths,
          Qt::QueuedConnection);
  connect(model, &QAbstractItemModel::columnsAboutToBeRemoved, this, &TableView::update_column_widths,
          Qt::QueuedConnection);
  QTableView::setModel(model);
}

void TableView::update_column_widths()
{
  if (model() == nullptr) {
    return;
  }
  QAbstractScrollArea::updateGeometry();
  const auto column_count = model()->columnCount();
  std::vector<int> size_hints;
  for (int i = 0; i < column_count; ++i) {
    size_hints.emplace_back(sizeHintForColumn(i));
  }

  const SizesCalculator sc(maximumViewportSize().width(), size_hints);
  const auto sizes = sc.sizes();
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
