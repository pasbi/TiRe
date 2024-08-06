#pragma once

#include <QTableView>

class TableView : public QTableView
{
  Q_OBJECT
public:
  explicit TableView(QWidget* parent = nullptr);
  void setModel(QAbstractItemModel* model) override;

protected:
  void resizeEvent(QResizeEvent* event) override;

private:
  void update_column_widths();
};

[[nodiscard]] TableView& setup_ui_with_single_table_view(QWidget* container);
