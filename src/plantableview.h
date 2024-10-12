#pragma once
#include "tableview.h"

class PlanTableView : public TableView
{
public:
  PlanTableView(QWidget* parent = nullptr);

private:
  void open_period_edit(const QModelIndex& index);
  std::unique_ptr<QAbstractItemDelegate> m_period_delegate;
  std::unique_ptr<QAbstractItemDelegate> m_kind_delegate;
};
