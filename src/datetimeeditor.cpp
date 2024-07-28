#include "datetimeeditor.h"
#include "ui_datetimeeditor.h"

#include <QDateTime>

DateTimeEditor::DateTimeEditor(QWidget* parent) : QDialog(parent), m_ui(std::make_unique<Ui::DateTimeEditor>())
{
  m_ui->setupUi(this);
  connect(m_ui->pb_now, &QPushButton::clicked, this, [this]() { set_time(QTime::currentTime()); });
}

DateTimeEditor::~DateTimeEditor() = default;

QDateTime DateTimeEditor::date_time() const
{
  const QTime time(m_ui->sp_h->value(), m_ui->sp_m->value());
  return QDateTime(m_ui->calendarWidget->selectedDate(), time);
}

void DateTimeEditor::set_time(const QTime& time)
{
  if (!time.isValid()) {
    set_time(QTime::currentTime());
    return;
  }

  m_ui->sp_h->setValue(time.hour());
  m_ui->sp_m->setValue(time.minute());
}

void DateTimeEditor::set_date(const QDate& date)
{
  if (!date.isValid()) {
    set_date(QDate::currentDate());
    return;
  }

  m_ui->calendarWidget->setSelectedDate(date);
}
