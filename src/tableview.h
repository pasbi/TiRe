#pragma once

#include <QTableView>

class TableView : public QTableView
{
  Q_OBJECT
public:
  using QTableView::QTableView;

protected:
  void resizeEvent(QResizeEvent* event) override;
};
