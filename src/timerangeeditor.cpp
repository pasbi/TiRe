#include "timerangeeditor.h"
#include "ui_timerangeeditor.h"

TimeRangeEditor::TimeRangeEditor(QWidget* parent) : QDialog(parent), m_ui(std::make_unique<Ui::TimeRangeEditor>())
{
  m_ui->setupUi(this);
  connect(m_ui->trs, &TimeRangeSlider::begin_changed, m_ui->te_begin, &QTimeEdit::setTime);
  connect(m_ui->trs, &TimeRangeSlider::end_changed, m_ui->te_end, &QTimeEdit::setTime);
  connect(m_ui->trs, &TimeRangeSlider::mid_changed, m_ui->te_mid, &QTimeEdit::setTime);
  connect(m_ui->te_begin, &QTimeEdit::timeChanged, m_ui->trs, &TimeRangeSlider::set_begin);
  connect(m_ui->te_end, &QTimeEdit::timeChanged, m_ui->trs, &TimeRangeSlider::set_end);
  connect(m_ui->te_mid, &QTimeEdit::timeChanged, m_ui->trs, &TimeRangeSlider::set_mid);
  set_split(false);
}

TimeRangeEditor::~TimeRangeEditor() = default;

void TimeRangeEditor::set_range(const QDateTime& begin, const QDateTime& end)
{
  m_ui->cw_begin->setSelectedDate(begin.date());
  m_ui->cw_end->setSelectedDate(end.date());
  // TODO if begin.date() != end.date(), the time range must be less restrictive.
  m_ui->te_begin->setTime(begin.time());
  m_ui->te_end->setTime(end.time());
  m_ui->cw_mid->setMinimumDate(begin.date());
  m_ui->cw_mid->setMaximumDate(end.date());
  using std::chrono_literals::operator""ms;
  const auto mid = begin.addMSecs((end - begin) / 2ms);
  m_ui->cw_mid->setSelectedDate(mid.date());
  m_ui->te_mid->setTime(mid.time());
}

QDateTime TimeRangeEditor::begin() const noexcept
{
  return {m_ui->cw_begin->selectedDate(), m_ui->te_begin->time()};
}

QDateTime TimeRangeEditor::end() const noexcept
{
  return {m_ui->cw_end->selectedDate(), m_ui->te_end->time()};
}

QDateTime TimeRangeEditor::mid() const noexcept
{
  return {m_ui->cw_mid->selectedDate(), m_ui->te_mid->time()};
}

void TimeRangeEditor::set_split(const bool split) const
{
  m_ui->trs->set_split(split);
  m_ui->cw_mid->setVisible(split);
  m_ui->te_mid->setVisible(split);
  m_ui->te_begin->setEnabled(!split);
  m_ui->te_end->setEnabled(!split);
  m_ui->cw_begin->setEnabled(!split);
  m_ui->cw_end->setEnabled(!split);
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
