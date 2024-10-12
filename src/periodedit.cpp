#include "periodedit.h"
#include "ui_periodedit.h"

#include <spdlog/spdlog.h>

PeriodEdit::PeriodEdit(QWidget* parent) : QDialog(parent), m_ui(std::make_unique<Ui::PeriodEdit>())
{
  m_ui->setupUi(this);
  connect(m_ui->de_begin, &QDateEdit::dateChanged, this, [this](const QDate& date) {
    const QSignalBlocker _(m_ui->de_end);
    m_ui->de_end->setMinimumDate(date);
    if (const auto type = this->type(); type != Period::Type::Custom) {
      const Period period(date, type);
      m_ui->de_end->setDate(period.end());
    }
  });
  connect(m_ui->de_end, &QDateEdit::dateChanged, this, [this](const QDate& date) {
    set_type(Period::Type::Custom);
    m_ui->de_begin->setMaximumDate(date);
  });
  connect(m_ui->cb_type, &QComboBox::currentIndexChanged, this,
          [this](const int index) { set_type(static_cast<Period::Type>(index)); });
}

PeriodEdit::~PeriodEdit() = default;

void PeriodEdit::set_period(const Period& period) const
{
  set_type(period.type());
  m_ui->de_begin->setDate(period.begin());
  m_ui->de_end->setDate(period.end());
}

void PeriodEdit::set_type(const Period::Type type) const
{
  m_ui->cb_type->setCurrentIndex(static_cast<int>(type));
  if (type != Period::Type::Custom) {
    const QSignalBlocker _1(m_ui->de_begin);
    const QSignalBlocker _2(m_ui->de_end);
    const Period period(m_ui->de_begin->date(), type);
    m_ui->de_begin->setDate(period.begin());
    m_ui->de_end->setDate(period.end());
  }
}

Period::Type PeriodEdit::type() const noexcept
{
  return static_cast<Period::Type>(m_ui->cb_type->currentIndex());
}

Period PeriodEdit::period() const noexcept
{
  if (const auto type = this->type(); type != Period::Type::Custom) {
    return Period{m_ui->de_begin->date(), type};
  }
  return Period{m_ui->de_begin->date(), m_ui->de_end->date()};
}
