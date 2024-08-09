#pragma once

#include <QTime>
#include <QWidget>

class TimeEdit : public QWidget
{
  Q_OBJECT
public:
  explicit TimeEdit(QWidget* parent = nullptr);
  ~TimeEdit();
  [[nodiscard]] QTime time() const;
  void set_time(const QTime& time);
  class Item;
  class NumberItem;
  class AMPMItem;
  [[nodiscard]] QTime current_time() const;

protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

Q_SIGNALS:
  void time_changed(QTime);

private:
  QTime m_current_time;

  NumberItem* m_hour_item = nullptr;
  NumberItem* m_minute_item = nullptr;
  NumberItem* m_current_item = nullptr;
  AMPMItem* m_ampm_item = nullptr;
  const std::vector<std::unique_ptr<Item>> m_items;
  const std::vector<NumberItem*> m_moveable_items;
  QTransform m_transform;
  std::mutex m_update_canvas_mutex;
};
