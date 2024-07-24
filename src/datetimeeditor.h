#pragma once

#include <QDialog>
#include <qdatetime.h>

namespace Ui
{
class DateTimeEditor;
}  // namespace Ui

class DateTimeEditor : public QDialog
{
  Q_OBJECT

public:
  explicit DateTimeEditor(QWidget* parent = nullptr);
  ~DateTimeEditor() override;

  [[nodiscard]] QDateTime date_time() const;
  void set_time(const QTime& time);
  void set_date(const QDate& date);

private:
  std::unique_ptr<Ui::DateTimeEditor> m_ui;
};
