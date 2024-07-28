#pragma once

#include "interval.h"
#include "project.h"
#include <QAbstractTableModel>
#include <deque>
#include <nlohmann/json_fwd.hpp>

class Period;

class Model : public QAbstractTableModel
{
public:
  explicit Model(std::vector<std::unique_ptr<Project>> projects, std::deque<std::unique_ptr<Interval>> intervals);
  explicit Model() = default;

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
  void add_project(std::unique_ptr<Project> project);

  [[nodiscard]] std::chrono::minutes minutes(const Period& period, const Project* project = nullptr) const;

  void new_interval();
  void add_interval(std::unique_ptr<Interval> interval);
  [[nodiscard]] std::vector<Project*> projects() const noexcept;

  void set_intervals(std::deque<std::unique_ptr<Interval>> intervals);
  [[nodiscard]] std::vector<Interval*> intervals() const;

private:
  std::deque<std::unique_ptr<Interval>> m_intervals;
  std::vector<std::unique_ptr<Project>> m_projects = {nullptr};

  // void set_project(Interval& interval, QString project);
  [[nodiscard]] QVariant background_data(const QModelIndex& index) const;
};
