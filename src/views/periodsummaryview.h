#pragma once

#include "period.h"
#include "views/abstractperiodview.h"
#include <set>

class PeriodSummaryModel;
class QTableView;

class PeriodSummaryView final : public AbstractPeriodView
{
  Q_OBJECT

public:
  explicit PeriodSummaryView(QWidget* parent = nullptr);
  ~PeriodSummaryView() override;
  void invalidate() override;

private:
  QTableView& m_table_view;
  std::unique_ptr<PeriodSummaryModel> m_model;
};
