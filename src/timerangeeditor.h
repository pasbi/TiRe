#pragma once

#include <QDialog>

namespace Ui
{
class TimeRangeEditor;
}  // namespace Ui

class TimeRangeEditor : public QDialog
{
public:
  TimeRangeEditor(QWidget* parent = nullptr);
  ~TimeRangeEditor() override;

  void set_range(const QDateTime& begin, const QDateTime& end);
  [[nodiscard]] QDateTime begin() const noexcept;
  [[nodiscard]] QDateTime end() const noexcept;

private:
  std::unique_ptr<Ui::TimeRangeEditor> m_ui;
  void sync() const;
};
