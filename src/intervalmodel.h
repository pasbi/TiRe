#pragma once

#include "interval.h"
#include "period.h"
#include "project.h"
#include <QAbstractTableModel>
#include <deque>

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
  [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
  [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
  [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
  [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
  [[nodiscard]] QModelIndex index(const Interval& interval) const;
  using QAbstractTableModel::index;
  Interval& remove_const(const Interval& interval) const;

  [[nodiscard]] std::chrono::minutes minutes(const std::optional<Period>& period = std::nullopt,
                                             const std::optional<QString>& name = std::nullopt) const;
  [[nodiscard]] std::chrono::minutes minutes(const QDate& date,
                                             const std::optional<QString>& name = std::nullopt) const;

  void add(std::unique_ptr<Interval> interval);
  std::unique_ptr<Interval> extract(const Interval& interval);
  void split_interval(const Interval& interval, const QDateTime& split_point);

  void set_intervals(std::deque<std::unique_ptr<Interval>> intervals);
  [[nodiscard]] std::vector<Interval*> intervals() const;
  [[nodiscard]] std::vector<Interval*> intervals(const Period& period) const;
  [[nodiscard]] const Interval* interval(std::size_t index) const;
  [[nodiscard]] std::vector<Interval*> open_intervals() const;

Q_SIGNALS:
  void data_changed();

private:
  std::deque<std::unique_ptr<Interval>> m_intervals;
  [[nodiscard]] QVariant background_data(const QModelIndex& index) const;
};

using DatePair = std::pair<QDate, QDate>;
Q_DECLARE_METATYPE(DatePair);
