#include "timeedit.h"
#include "ui_timeedit.h"

TimeEdit::TimeEdit(QWidget* parent) : m_ui(std::make_unique<Ui::TimeEdit>())
{
  m_ui->setupUi(this);
  connect(m_ui->sp_min, &TimeEditSpinBox::wrapped, m_ui->sp_h, &QSpinBox::stepBy);
  connect(m_ui->sp_min, &QSpinBox::valueChanged, this, &TimeEdit::time_changed);
  connect(m_ui->sp_h, &QSpinBox::valueChanged, this, &TimeEdit::time_changed);
}

TimeEdit::~TimeEdit() = default;

QTime TimeEdit::time() const
{
  return QTime(m_ui->sp_h->value(), m_ui->sp_min->value());
}

void TimeEdit::set_time(const QTime& time) noexcept
{
  if (time == this->time()) {
    return;
  }

  {
    const QSignalBlocker blocker(this);
    m_ui->sp_h->setValue(time.hour());
    m_ui->sp_min->setValue(time.minute());
  }
  Q_EMIT time_changed();
}
