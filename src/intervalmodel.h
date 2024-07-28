#pragma once

#include "interval.h"
#include "project.h"
#include <QAbstractTableModel>
#include <deque>

class Period;

class IntervalModel final : public QAbstractTableModel
{
  Q_OBJECT
public:
  explicit IntervalModel(std::deque<std::unique_ptr<Interval>> intervals);
  explicit IntervalModel() = default;

  static constexpr auto project_column = 0;
  static constexpr auto date_column = 1;
  static constexpr auto begin_column = 2;
  static constexpr auto end_column = 3;
  static constexpr auto duration_column = 4;
  [[nodiscard]] int rowCount(const QModelIndex& parent) const override;
  [[nodiscard]] int columnCount(const QModelIndex& parent) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
  [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
  [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
  // bool setData(const QModelIndex& index, const QVariant& value, int role) override;

  [[nodiscard]] std::chrono::minutes minutes(const Period& period,
                                             const std::optional<Project::Type>& type = std::nullopt,
                                             const std::optional<QString>& name = std::nullopt) const;

  void new_interval(const Project& project);
  void add_interval(std::unique_ptr<Interval> interval);

  void set_intervals(std::deque<std::unique_ptr<Interval>> intervals);
  [[nodiscard]] std::vector<Interval*> intervals() const;

Q_SIGNALS:
  void data_changed();

private:
  std::deque<std::unique_ptr<Interval>> m_intervals;

  // void set_project(Interval& interval, QString project);
  [[nodiscard]] QVariant background_data(const QModelIndex& index) const;
};
