#pragma once

#include "period.h"
#include "timesheet.h"

#include <QAbstractTableModel>

class Project;
class TimeSheet;
class PeriodSummaryModel : public QAbstractTableModel
{

public:
  [[nodiscard]] int rowCount(const QModelIndex& parent) const override;
  [[nodiscard]] int columnCount(const QModelIndex& parent) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
  [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

  void set_source(const TimeSheet* model);
  void set_period(const Period& period);

private:
  const TimeSheet* m_time_sheet = nullptr;
  [[nodiscard]] const Project* project(const int row) const noexcept;
  [[nodiscard]] QDate date(const int column) const noexcept;
  void update_summary();
  std::map<std::pair<const Project*, QDate>, std::chrono::minutes> m_minutes;
  Period m_period;
};
