#pragma once
#include "tableview.h"

class PlanTableView : public TableView
{
public:
  explicit PlanTableView(QWidget* parent = nullptr);

private:
  void open_period_edit(const QModelIndex& index);
  void delete_selected_entries();
  std::unique_ptr<QAbstractItemDelegate> m_period_delegate;
  std::unique_ptr<QAbstractItemDelegate> m_kind_delegate;
};
