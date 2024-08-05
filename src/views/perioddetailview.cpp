#include "views/perioddetailview.h"
#include "intervalmodel.h"
#include "plan.h"
#include "tableview.h"
#include "views/abstractperiodproxymodel.h"
#include <QHBoxLayout>
#include <spdlog/spdlog.h>

namespace
{

class ProxyModel final : public AbstractPeriodProxyModel
{
public:
  using AbstractPeriodProxyModel::AbstractPeriodProxyModel;

protected:
  [[nodiscard]] bool filterAcceptsRow(const int source_row, const QModelIndex& source_parent) const override
  {
    if (interval_model() == nullptr) {
      return false;
    }
    const auto* const interval = interval_model()->intervals().at(source_row);
    const Period period(interval->begin().date(),
                        (interval->end().isValid() ? interval->end() : interval->begin()).date());
    return current_period().contains(period);
  }
};

}  // namespace

PeriodDetailView::PeriodDetailView(QWidget* parent)
  : AbstractPeriodView(std::make_unique<::ProxyModel>(), parent), m_table_view(::setup_ui_with_single_table_view(this))

{
  connect(&m_table_view, &QAbstractItemView::doubleClicked, this,
          [this](const QModelIndex& index) { Q_EMIT double_clicked(proxy_model()->mapToSource(index)); });
  m_table_view.setModel(proxy_model());
}

PeriodDetailView::~PeriodDetailView() = default;

const Interval* PeriodDetailView::current_interval() const
{
  if (const auto index = m_table_view.currentIndex(); index.isValid()) {
    const auto row = proxy_model()->mapToSource(index).row();
    return interval_model()->interval(row);
  }
  return nullptr;
}

std::set<const Interval*> PeriodDetailView::selected_intervals() const
{
  std::set<const Interval*> selection;
  for (const auto& index : m_table_view.selectionModel()->selectedRows()) {
    const auto row = proxy_model()->mapToSource(index).row();
    selection.insert(interval_model()->interval(row));
  }
  return selection;
}

void PeriodDetailView::invalidate()
{
  update();
}
