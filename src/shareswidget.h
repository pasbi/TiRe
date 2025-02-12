#pragma once

#include <QWidget>

class TimeSheet;
class Period;
class Project;

class SharesWidget : public QWidget
{
public:
  explicit SharesWidget(QWidget* parent = nullptr);
  void update(const TimeSheet& time_sheet, const Period& period);
  using QWidget::update;

protected:
  void paintEvent(QPaintEvent* event) override;

private:
  std::vector<std::pair<const Project*, double>> m_shares;
};
