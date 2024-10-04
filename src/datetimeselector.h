#pragma once
#include <QDateTime>
#include <QWidget>

class DateTimeSelector : public QWidget
{
  Q_OBJECT
public:
  explicit DateTimeSelector(QWidget* parent = nullptr);
  void go_left();
  void go_right();
  [[nodiscard]] QDateTime date_time() const;
  void set_date_time(const QDateTime& date_time);
  void set_minimum_date_time(const QDateTime& minimum_date_time);
  void set_maximum_date_time(const QDateTime& maximum_date_time);

Q_SIGNALS:
  void left_fence_reached_changed(bool);
  void right_fence_reached_changed(bool);
  void date_time_changed(QDateTime);

protected:
  void paintEvent(QPaintEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

private:
  struct MousePressState
  {
    QPoint pos;
    QDateTime left_fence;
  };
  std::optional<MousePressState> m_mouse_press_state;
  QDateTime m_current_date_time;
  QDateTime m_minimum_date_time;
  QDateTime m_maximum_date_time;
  QDateTime m_left_fence;
  QDateTime m_right_fence;
  [[nodiscard]] std::chrono::minutes visible_range() const;
  [[nodiscard]] std::chrono::minutes advance() const;
  void set_left_fence(const QDateTime& left_fence);
  [[nodiscard]] static std::chrono::minutes px_to_min(int px);
  QDateTime constrain(QDateTime date_time, const std::chrono::minutes& overshoot = {}) const;
  int m_accumulated_wheel_y = 0;
};
