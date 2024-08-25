#include "timerangeeditor.h"
#include "ui_timerangeeditor.h"

TimeRangeEditor::TimeRangeEditor(QWidget* parent) : QDialog(parent), m_ui(std::make_unique<Ui::TimeRangeEditor>())
{
  m_ui->setupUi(this);
  connect(m_ui->trs, &TimeRangeSlider::begin_changed, m_ui->te_begin, &QTimeEdit::setTime);
  connect(m_ui->trs, &TimeRangeSlider::end_changed, m_ui->te_end, &QTimeEdit::setTime);
  connect(m_ui->te_begin, &QTimeEdit::timeChanged, m_ui->trs, &TimeRangeSlider::set_begin);
  connect(m_ui->te_end, &QTimeEdit::timeChanged, m_ui->trs, &TimeRangeSlider::set_end);
}

TimeRangeEditor::~TimeRangeEditor() = default;

void TimeRangeEditor::set_range(const QDateTime& begin, const QDateTime& end)
{
  m_ui->cw_begin->setSelectedDate(begin.date());
  m_ui->cw_end->setSelectedDate(end.date());
  m_ui->te_begin->setTime(begin.time());
  m_ui->te_end->setTime(end.time());
  sync();
}

QDateTime TimeRangeEditor::begin() const noexcept
{
  return {m_ui->cw_begin->selectedDate(), m_ui->te_begin->time()};
}

QDateTime TimeRangeEditor::end() const noexcept
{
  return {m_ui->cw_end->selectedDate(), m_ui->te_end->time()};
}

void TimeRangeEditor::sync() const
{
  m_ui->cw_begin->setMaximumDate(m_ui->cw_end->selectedDate());
  m_ui->cw_end->setMinimumDate(m_ui->cw_begin->selectedDate());
  if (begin().date() == end().date()) {
    // m_ui->te_begin->setMaximumTime(m_ui->te_end->time());
    // m_ui->te_end->setMinimumTime(m_ui->te_begin->time());
    // TODO sync time range slider
  }
}
