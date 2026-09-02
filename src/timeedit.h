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

  /**
   * @name Focus chain endpoints
   * TimeEdit is a compound widget whose focusable children live in its own .ui file, so a
   * containing dialog's <tabstops> cannot name them and they end up stranded at the end of the
   * focus chain. These let the dialog weave the widget into its tab order with setTabOrder().
   * @{
   */
  [[nodiscard]] QWidget* first_focus_widget() const;
  [[nodiscard]] QWidget* last_focus_widget() const;
  /** @} */

Q_SIGNALS:
  void time_changed();

private:
  std::unique_ptr<Ui::TimeEdit> m_ui;
  QTime m_min;
  QTime m_max;

  void handle_change();
};
