#include "timerangeeditor.h"

#include "application.h"
#include "ui_timerangeeditor.h"

#include <QMessageBox>

TimeRangeEditor::TimeRangeEditor(QWidget* const parent) : QDialog(parent), m_ui(std::make_unique<Ui::TimeRangeEditor>())
{
  m_ui->setupUi(this);

  // The .ui file's <tabstops> chain de_begin -> cb_has_end -> sp_end_offset. The two TimeEdits
  // cannot be named there: they are compound widgets whose focusable children live in
  // timeedit.ui, so they would otherwise be stranded at the end of the chain, after the buttons
  // and in reverse order. Weave them in here so focus follows the visual grid.
  setTabOrder(m_ui->de_begin, m_ui->te_begin->first_focus_widget());
  setTabOrder(m_ui->te_begin->first_focus_widget(), m_ui->te_begin->last_focus_widget());
  setTabOrder(m_ui->te_begin->last_focus_widget(), m_ui->cb_has_end);
  setTabOrder(m_ui->sp_end_offset, m_ui->te_end->first_focus_widget());
  setTabOrder(m_ui->te_end->first_focus_widget(), m_ui->te_end->last_focus_widget());
  setTabOrder(m_ui->te_end->last_focus_widget(), m_ui->buttonBox);

  connect(m_ui->cb_has_end, &QCheckBox::toggled, this, &TimeRangeEditor::update_enabledness);
  update_enabledness();
}

TimeRangeEditor::~TimeRangeEditor() = default;

void TimeRangeEditor::set_range(const QDateTime& begin, const QDateTime& end)
{
  const auto proposed_end = end.isValid() ? end : Application::current_date_time();
  m_ui->te_begin->set_time(begin.time());
  m_ui->de_begin->setDate(begin.date());
  m_ui->te_end->set_time(proposed_end.time());
  m_ui->sp_end_offset->setValue(static_cast<int>(begin.date().daysTo(proposed_end.date())));
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

void TimeRangeEditor::focus_begin_time()
{
  m_ui->te_begin->first_focus_widget()->setFocus();
}

void TimeRangeEditor::focus_end_time()
{
  // Only has an effect while the end is enabled, i.e. the interval is not marked as running.
  m_ui->te_end->first_focus_widget()->setFocus();
}

void TimeRangeEditor::update_enabledness() const
{
  for (auto* const widget : std::vector<QWidget*>{m_ui->te_end, m_ui->sp_end_offset}) {
    widget->setEnabled(m_ui->cb_has_end->isChecked());
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
