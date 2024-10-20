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
  void current_interval_changed(const Interval* interval);

private:
  QTableView& m_table_view;
  std::unique_ptr<PeriodDetailProxyModel> m_proxy_model;
  std::unique_ptr<QAbstractItemDelegate> m_ro_item_delegate;
  std::unique_ptr<QAbstractItemDelegate> m_begin_end_delegate;
  std::unique_ptr<QAbstractItemDelegate> m_project_delegate;
  std::vector<std::unique_ptr<QAction>> m_context_menu_actions;

  void init_context_menu_actions();
  void show_table_context_menu(const QPoint& pos);
  void edit_date_time(const QModelIndex& index) const;
  void edit_project(const QModelIndex& index) const;
};
