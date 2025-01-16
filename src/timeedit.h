#pragma once

#include <QTime>
#include <QWidget>

namespace Ui
{
class TimeEdit;
}  // namespace Ui

class TimeEdit : public QWidget
{
  Q_OBJECT
public:
  explicit TimeEdit(QWidget* parent = nullptr);
  ~TimeEdit() override;

  [[nodiscard]] QTime time() const;
  void set_time(const QTime& time) noexcept;
  void set_time_range(const QTime& min, const QTime& max) noexcept;

Q_SIGNALS:
  void time_changed();

private:
  std::unique_ptr<Ui::TimeEdit> m_ui;
  QTime m_min;
  QTime m_max;

  void handle_change();
};
