#include "timeedit.h"
#include "ui_timeedit.h"

TimeEdit::TimeEdit(QWidget* parent) : m_ui(std::make_unique<Ui::TimeEdit>())
{
  m_ui->setupUi(this);
  connect(m_ui->sp_min, &TimeEditSpinBox::wrapped, m_ui->sp_h, &QSpinBox::stepBy);
  connect(m_ui->sp_min, &QSpinBox::valueChanged, this, &TimeEdit::handle_change);
  connect(m_ui->sp_h, &QSpinBox::valueChanged, this, &TimeEdit::handle_change);
  set_time_range({}, {});
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

void TimeEdit::set_time_range(const QTime& min, const QTime& max) noexcept
{
  m_min = min;
  m_max = max;
  const auto min_h = m_min.isValid() ? m_min.hour() : 0;
  const auto max_h = m_max.isValid() ? m_max.hour() : 23;
  m_ui->sp_h->setRange(min_h, max_h);
}

void TimeEdit::handle_change()
{
  Q_EMIT time_changed();

  const auto h = m_ui->sp_h->value();
  const auto min_m = h == m_min.hour() ? m_min.minute() : 0;
  const auto max_m = h == m_max.hour() ? m_max.minute() : 59;
  m_ui->sp_min->set_block_wrap_down(h == m_min.hour());
  m_ui->sp_min->set_block_wrap_up(h == m_max.hour());
  m_ui->sp_min->setRange(min_m, max_m);
}
