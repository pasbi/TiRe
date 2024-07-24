#pragma once

#include "interval.h"
#include <QAbstractTableModel>
#include <deque>
#include <filesystem>
#include <set>

class Model : public QAbstractTableModel
{
public:
  static constexpr auto project_column = 0;
  static constexpr auto begin_column = 1;
  static constexpr auto end_column = 2;
  static constexpr auto duration_column = 3;
  void save(const std::filesystem::path& path);
  void load(const std::filesystem::path& path);
  [[nodiscard]] int rowCount(const QModelIndex& parent) const override;
  [[nodiscard]] int columnCount(const QModelIndex& parent) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
  [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
  [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role) override;

  void new_interval();
  void add_interval(Interval interval);
  [[nodiscard]] const QStringList& projects() const noexcept;

private:
  std::deque<Interval> m_intervals;
  QStringList m_projects;

  void set_project(Interval& interval, QString project);
};
