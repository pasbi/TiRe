#pragma once

#include <QDateTime>
#include <QDialog>

namespace Ui
{
class TimeRangeEditor;
}

class TimeRangeEditor : public QDialog
{
  Q_OBJECT

public:
  explicit TimeRangeEditor(QWidget* parent);
  ~TimeRangeEditor() override;

  void set_range(const QDateTime& begin, const QDateTime& end);
  [[nodiscard]] QDateTime begin() const noexcept;
  [[nodiscard]] QDateTime end() const noexcept;
  void focus_begin_time();
  void focus_end_time();
  void accept() override;

private:
  std::unique_ptr<Ui::TimeRangeEditor> m_ui;
  void update_enabledness() const;
};
