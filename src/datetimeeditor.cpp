#include "datetimeeditor.h"
#include "ui_datetimeeditor.h"
#include <QDateTime>

DateTimeEditor::DateTimeEditor(QWidget* parent) : QDialog(parent), m_ui(std::make_unique<Ui::DateTimeEditor>())
{
  m_ui->setupUi(this);
  connect(m_ui->pb_now, &QPushButton::clicked, this, [this]() {
    set_time(QTime::currentTime());
    set_date(QDate::currentDate());
  });
  connect(m_ui->timeEdit, &QTimeEdit::timeChanged, m_ui->time_edit, &TimeEdit::set_time);
  connect(m_ui->time_edit, &TimeEdit::time_changed, m_ui->timeEdit, &QTimeEdit::setTime);
}

DateTimeEditor::~DateTimeEditor() = default;

QDateTime DateTimeEditor::date_time() const
{
  return {m_ui->calendarWidget->selectedDate(), m_ui->time_edit->time()};
}

void DateTimeEditor::set_time(const QTime& time)
{
  if (!time.isValid()) {
    set_time(QTime::currentTime());
    return;
  }

  m_ui->time_edit->set_time(time);
}

void DateTimeEditor::set_date(const QDate& date)
{
  if (!date.isValid()) {
    set_date(QDate::currentDate());
    return;
  }

  m_ui->calendarWidget->setSelectedDate(date);
}
