#include "timerangeeditor.h"

#include "application.h"
#include "datetimeselector.h"
#include "ui_timerangeeditor.h"

#include <QMessageBox>

TimeRangeEditor::TimeRangeEditor(QWidget* const parent)
  : QDialog(parent), m_ui(std::make_unique<Ui::TimeRangeEditor>())
{
  m_ui->setupUi(this);
  connect(m_ui->cb_has_end, &QCheckBox::toggled, this, &TimeRangeEditor::update_enabledness);
  connect(m_ui->pb_begin_to_last_end, &QPushButton::clicked, this, []() {});
  connect(m_ui->pb_begin_to_now, &QPushButton::clicked, this, [this]() {
    m_ui->te_begin->setDate(Application::current_date_time().date());
    m_ui->de_begin->setTime(Application::current_date_time().time());
  });
  connect(m_ui->pb_end_to_begin, &QPushButton::clicked, this, [this]() {
    m_ui->te_end->setTime(m_ui->te_begin->time());
    m_ui->sp_end_offset->setValue(0);
  });
  connect(m_ui->pb_end_to_now, &QPushButton::clicked, this, [this]() {
    const auto offset = m_ui->te_begin->date().daysTo(Application::current_date_time().date());
    m_ui->sp_end_offset->setValue(offset);
    m_ui->te_end->setTime(Application::current_date_time().time());
  });
  update_enabledness();
}

TimeRangeEditor::~TimeRangeEditor() = default;

void TimeRangeEditor::set_range(const QDateTime& begin, const QDateTime& end)
{
  m_ui->te_begin->setTime(begin.time());
  m_ui->de_begin->setDate(begin.date());
  m_ui->te_end->setTime(end.time());
  m_ui->sp_end_offset->setValue(begin.date().daysTo(end.date()));
  const auto e = end.isValid() ? end : Application::current_date_time();
  m_ui->cb_has_end->setChecked(end.isValid());
  update();
}

QDateTime TimeRangeEditor::begin() const noexcept
{
  return {m_ui->de_begin->date(), m_ui->te_begin->time()};
}

QDateTime TimeRangeEditor::end() const noexcept
{
  return m_ui->cb_has_end->isChecked()
             ? QDateTime{m_ui->de_begin->date().addDays(m_ui->sp_end_offset->value()), m_ui->te_end->time()}
             : QDateTime{};
}

void TimeRangeEditor::set_end(const QDateTime& end)
{
  set_range(begin(), end);
}

void TimeRangeEditor::update_enabledness() const
{
  for (auto* const w :
       std::vector<QWidget*>{m_ui->pb_end_to_begin, m_ui->pb_end_to_now, m_ui->te_end, m_ui->sp_end_offset})
  {
    w->setEnabled(m_ui->cb_has_end->isChecked());
  }
}

void TimeRangeEditor::accept()
{
  if (const auto end = this->end(); !end.isValid() || begin() <= end) {
    QDialog::accept();
  } else {
    QMessageBox::warning(this, QApplication::applicationDisplayName(),
                         tr("Invalid Time Range: End must not be after begin."));
  }
}
