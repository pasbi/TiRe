#pragma once

#include <QTableView>

class TableView : public QTableView
{
public:
  Q_OBJECT
  using QTableView::QTableView;

protected:
  void resizeEvent(QResizeEvent* event) override;
};