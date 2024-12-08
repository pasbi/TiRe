#include "splitpointeditor.h"

#include "ui_splitpointeditor.h"

SplitPointEditor::SplitPointEditor(QWidget* const parent)
  : QDialog(parent), m_ui(std::make_unique<Ui::SplitPointEditor>())
{
  m_ui->setupUi(this);
  connect(m_ui->de_split, &QDateEdit::dateChanged, this, [this](const QDate& date) {
    m_ui->te_split->setTimeRange(date == m_lower.date() ? m_lower.time() : date.startOfDay().time(),
                                 date == m_upper.date() ? m_upper.time() : date.endOfDay().time());
    update_percent();
  });
  connect(m_ui->te_split, &QTimeEdit::timeChanged, this, &SplitPointEditor::update_percent);
  connect(m_ui->sp_split, &QDoubleSpinBox::valueChanged, this, &SplitPointEditor::update_date_time);
}

SplitPointEditor::~SplitPointEditor() = default;

void SplitPointEditor::set_range(const QDateTime& begin, const QDateTime& end)
{
  m_ui->le_begin->setText(begin.toString());
  m_ui->le_end->setText(end.toString());
  m_lower = begin;
  m_upper = end;
  m_ui->de_split->setDateRange(begin.date(), end.date());
  update();
}

QDateTime SplitPointEditor::split_point() const noexcept
{
  return {m_ui->de_split->date(), m_ui->te_split->time()};
}

void SplitPointEditor::update_percent() const
{
  const auto total = static_cast<double>(m_lower.msecsTo(m_upper));
  const auto t = static_cast<double>(m_lower.msecsTo(split_point()));
  m_ui->sp_split->setValue(t / total * 100.0);
}

void SplitPointEditor::update_date_time() const
{
  const auto total = static_cast<double>(m_lower.msecsTo(m_upper));
  const auto split_point = m_lower.addMSecs(total * m_ui->sp_split->value() / 100.0);
  m_ui->de_split->setDate(split_point.date());
  m_ui->te_split->setTime(split_point.time());
}
