#pragma once

#include "interval.h"
#include "period.h"
#include "project.h"
#include <QAbstractTableModel>
#include <deque>

class AbstractTimeSheetRepository;

class IntervalModel final : public QAbstractTableModel
{
  Q_OBJECT
public:
  /** @brief Creates a model that does not persist anything. */
  explicit IntervalModel();
  explicit IntervalModel(AbstractTimeSheetRepository& repository);
  /** @brief Adopts already-stored intervals without writing them back. */
  explicit IntervalModel(AbstractTimeSheetRepository& repository, std::deque<std::unique_ptr<Interval>> intervals);

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

  /**
   * @brief Writes @p interval's current state to the store.
   * Interval's own setters cannot do this: an Interval is routinely filled in before it belongs
   * to a model, when there is no row yet. Call this after mutating an owned interval --
   * make_modify_interval_command() does so for every undoable edit.
   */
  void persist(const Interval& interval);

  void set_intervals(std::deque<std::unique_ptr<Interval>> intervals);
  [[nodiscard]] std::vector<Interval*> intervals() const;
  [[nodiscard]] std::vector<Interval*> intervals(const Period& period) const;
  [[nodiscard]] const Interval* interval(std::size_t index) const;
  [[nodiscard]] std::vector<Interval*> open_intervals() const;

Q_SIGNALS:
  void data_changed();

private:
  AbstractTimeSheetRepository& m_repository;
  std::deque<std::unique_ptr<Interval>> m_intervals;
  [[nodiscard]] QVariant background_data(const QModelIndex& index) const;
};

using DatePair = std::pair<QDate, QDate>;
Q_DECLARE_METATYPE(DatePair);
