#include "views/perioddetailview.h"
#include "intervalmodel.h"
#include "plan.h"
#include "ui_perioddetailview.h"
#include "views/abstractperiodproxymodel.h"
#include <spdlog/spdlog.h>

namespace
{

[[nodiscard]] auto format_minutes(const std::chrono::minutes minutes)
{
  using std::chrono_literals::operator""h;
  using std::chrono_literals::operator""min;
  static constexpr auto field_width = 2;
  static constexpr auto base = 10;
  static constexpr auto fill_char = QChar('0');
  return QString{"%1%2:%3"}
      .arg(minutes < 0min ? "-" : "")
      .arg(std::abs(minutes / 1h), field_width, base, fill_char)
      .arg(std::abs((minutes / 1min)) % (1h / 1min), field_width, base, fill_char);
}

class ProxyModel final : public AbstractPeriodProxyModel
{
public:
  using AbstractPeriodProxyModel::AbstractPeriodProxyModel;

protected:
  [[nodiscard]] bool filterAcceptsRow(const int source_row, const QModelIndex& source_parent) const override
  {
    if (interval_model() == nullptr) {
      return false;
    }
    const auto* const interval = interval_model()->intervals().at(source_row);
    const Period period(interval->begin().date(),
                        (interval->end().isValid() ? interval->end() : interval->begin()).date());
    return current_period().contains(period);
  }
};

}  // namespace

PeriodDetailView::PeriodDetailView(QWidget* parent)
  : AbstractPeriodView(std::make_unique<::ProxyModel>(), parent), m_ui(std::make_unique<Ui::PeriodDetailView>())
{
  m_ui->setupUi(this);
  connect(m_ui->tableView, &QAbstractItemView::doubleClicked, this,
          [this](const QModelIndex& index) { Q_EMIT double_clicked(proxy_model()->mapToSource(index)); });
  m_ui->tableView->setModel(proxy_model());

  connect(this, &PeriodDetailView::period_changed, this,
          [this]() { m_ui->lb_current_period->setText(current_period().label()); });
}

PeriodDetailView::~PeriodDetailView() = default;

const Interval* PeriodDetailView::current_interval() const
{
  if (const auto index = m_ui->tableView->currentIndex(); index.isValid()) {
    const auto row = proxy_model()->mapToSource(index).row();
    return interval_model()->interval(row);
  }
  return nullptr;
}

std::set<const Interval*> PeriodDetailView::selected_intervals() const
{
  std::set<const Interval*> selection;
  for (const auto& index : m_ui->tableView->selectionModel()->selectedRows()) {
    const auto row = proxy_model()->mapToSource(index).row();
    selection.insert(interval_model()->interval(row));
  }
  return selection;
}

void PeriodDetailView::clear() const
{
  m_ui->lb_holiday->setText("-");
  m_ui->lb_total->setText("-");
  m_ui->lb_overtime->setText("-");
  m_ui->lb_overtime_cum->setText("-");
  m_ui->lb_planned->setText("-");
  m_ui->lb_sick->setText("-");
}

void PeriodDetailView::invalidate()
{
  if (interval_model() == nullptr) {
    clear();
    return;
  }

  // TODO split this into another class
  const auto actual_working_time = interval_model()->minutes(current_period(), Project::Type::Work);
  const auto planned_working_time = plan()->planned_working_time(current_period().begin(), current_period().end());
  m_ui->lb_total->setText(::format_minutes(actual_working_time));
  m_ui->lb_holiday->setText(::format_minutes(interval_model()->minutes(current_period(), Project::Type::Holiday)));
  m_ui->lb_sick->setText(::format_minutes(interval_model()->minutes(current_period(), Project::Type::Sick)));
  m_ui->lb_planned->setText(::format_minutes(planned_working_time));
  m_ui->lb_overtime->setText(::format_minutes(actual_working_time - planned_working_time));

  const auto total_overtime = plan()->overtime_offset() + interval_model()->minutes({}, Project::Type::Work)
                              - plan()->planned_working_time({}, QDate::currentDate());
  m_ui->lb_overtime_cum->setText(::format_minutes(total_overtime));

  update();
}
