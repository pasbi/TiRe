#include "timerangeeditor2.h"

#include "application.h"
#include "datetimeselector.h"
#include "ui_timerangeeditor2.h"

TimeRangeEditor2::TimeRangeEditor2(QWidget* parent) : QDialog(parent), m_ui(std::make_unique<Ui::TimeRangeEditor2>())
{
  m_ui->setupUi(this);
  connect(m_ui->dt_begin, &QDateTimeEdit::dateTimeChanged, this, &TimeRangeEditor2::update_fences);
  connect(m_ui->dt_end, &QDateTimeEdit::dateTimeChanged, this, &TimeRangeEditor2::update_fences);
  connect(m_ui->w_slider, &DateTimeSelector::date_time_changed, this, &TimeRangeEditor2::update_fences);
  connect(m_ui->w_slider, &DateTimeSelector::date_time_changed, m_ui->dt_end, &QDateTimeEdit::setDateTime);
  connect(m_ui->dt_end, &QDateTimeEdit::dateTimeChanged, m_ui->w_slider, &DateTimeSelector::set_date_time);
  connect(m_ui->cb_has_end, &QCheckBox::toggled, this, &TimeRangeEditor2::update_enabledness);
  connect(m_ui->pb_now, &QPushButton::clicked, this, [this]() { set_end(Application::current_date_time()); });
  connect(m_ui->pb_begin, &QPushButton::clicked, this, [this]() { set_end(begin()); });
  update_enabledness();
}

TimeRangeEditor2::~TimeRangeEditor2() = default;

void TimeRangeEditor2::set_range(const QDateTime& begin, const QDateTime& end)
{
  m_ui->dt_begin->setDateTime(begin);
  m_ui->w_slider->set_minimum_date_time(begin);
  const auto e = end.isValid() ? end : Application::current_date_time();
  m_ui->dt_end->setDateTime(e);
  m_ui->w_slider->set_date_time(e);
  m_ui->cb_has_end->setChecked(end.isValid());
  update();
}

QDateTime TimeRangeEditor2::begin() const noexcept
{
  return m_ui->dt_begin->dateTime();
}

QDateTime TimeRangeEditor2::end() const noexcept
{
  return m_ui->cb_has_end->isChecked() ? m_ui->dt_end->dateTime() : QDateTime{};
}

void TimeRangeEditor2::set_end(const QDateTime& end)
{
  set_range(begin(), end);
}

void TimeRangeEditor2::update_fences() const
{
  m_ui->dt_end->setMinimumDateTime(m_ui->dt_begin->dateTime());
  m_ui->dt_begin->setMaximumDateTime(m_ui->dt_end->dateTime());
}

void TimeRangeEditor2::update_enabledness() const
{
  for (auto* const w : std::vector<QWidget*>{m_ui->dt_end, m_ui->w_slider, m_ui->pb_now, m_ui->pb_begin}) {
    w->setEnabled(m_ui->cb_has_end->isChecked());
  }
}
