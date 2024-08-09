#pragma once

#include "period.h"
#include "timesheet.h"

#include <QAbstractTableModel>

class Project;
class TimeSheet;
class PeriodSummaryModel final : public QAbstractTableModel
{
public:
  explicit PeriodSummaryModel(QObject* parent = nullptr);
  ~PeriodSummaryModel() override;
  [[nodiscard]] int rowCount(const QModelIndex& parent) const override;
  [[nodiscard]] int columnCount(const QModelIndex& parent) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
  [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

  void set_source(const TimeSheet* model);
  void set_period(const Period& period);
  class ExtraRow;
  [[nodiscard]] std::chrono::minutes get_duration(const QDate& date, const Project* project = nullptr) const;
  [[nodiscard]] QDate date(int column) const noexcept;

private:
  const TimeSheet* m_time_sheet = nullptr;
  [[nodiscard]] const Project* project(int row) const noexcept;
  void update_summary();
  std::map<QDate, std::map<const Project*, std::chrono::minutes>> m_minutes;
  Period m_period;

  std::vector<std::unique_ptr<ExtraRow>> m_extra_rows;
};
