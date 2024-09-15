#include "datetimeeditor.h"
#include "application.h"
#include "ui_datetimeeditor.h"
#include <QDateTime>

DateTimeEditor::DateTimeEditor(QWidget* parent) : QDialog(parent), m_ui(std::make_unique<Ui::DateTimeEditor>())
{
  m_ui->setupUi(this);
  connect(m_ui->pb_now, &QPushButton::clicked, this, [this]() { set_date_time(Application::current_date_time()); });
  connect(m_ui->timeEdit, &QTimeEdit::timeChanged, m_ui->time_edit, &TimeEdit::set_time);
  connect(m_ui->time_edit, &TimeEdit::time_changed, m_ui->timeEdit, &QTimeEdit::setTime);
}

DateTimeEditor::~DateTimeEditor() = default;

QDateTime DateTimeEditor::date_time() const
{
  return {m_ui->calendarWidget->selectedDate(), m_ui->time_edit->time()};
}

void DateTimeEditor::set_date_time(const QDateTime& date_time) const
{
  m_ui->time_edit->set_time(date_time.time());
  m_ui->calendarWidget->setSelectedDate(date_time.date());
}

void DateTimeEditor::set_minimum_date_time(const QDateTime& date_time) const
{
  m_ui->calendarWidget->setMinimumDate(date_time.date());
}

void DateTimeEditor::set_maximum_date_time(const QDateTime& date_time) const
{
  m_ui->calendarWidget->setMaximumDate(date_time.date());
}
