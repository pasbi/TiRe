#pragma once

#include "period.h"
#include "views/abstractperiodview.h"
#include <set>

class QAbstractItemDelegate;
class PeriodDetailProxyModel;
class QTableView;

class PeriodDetailView final : public AbstractPeriodView
{
  Q_OBJECT

public:
  explicit PeriodDetailView(QWidget* parent = nullptr);
  ~PeriodDetailView() override;
  void invalidate() override;
  [[nodiscard]] const Interval* current_interval() const;
  [[nodiscard]] std::set<const Interval*> selected_intervals() const;
  void set_model(const TimeSheet* time_sheet) override;
  void set_period(const Period& period) override;

Q_SIGNALS:
  void double_clicked(const QModelIndex& index);
  void current_interval_changed(const Interval* interval);

private:
  QTableView& m_table_view;
  std::unique_ptr<PeriodDetailProxyModel> m_proxy_model;
  class ItemDelegate;
  std::unique_ptr<ItemDelegate> m_item_delegate;
};
