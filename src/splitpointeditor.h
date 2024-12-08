#pragma once

#include <QDateTime>
#include <QDialog>

namespace Ui
{
class SplitPointEditor;
}

class SplitPointEditor : public QDialog
{
  Q_OBJECT

public:
  explicit SplitPointEditor(QWidget* parent = nullptr);
  ~SplitPointEditor() override;

  void set_range(const QDateTime& begin, const QDateTime& end);
  [[nodiscard]] QDateTime split_point() const noexcept;

private:
  std::unique_ptr<Ui::SplitPointEditor> m_ui;
  QDateTime m_lower;
  QDateTime m_upper;
  void update_percent() const;
  void update_date_time() const;
};
