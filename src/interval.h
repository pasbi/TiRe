#pragma once

#include <QDateTime>
#include <compare>
#include <nlohmann/adl_serializer.hpp>

class Project;

class Interval
{
public:
  friend std::weak_ordering operator<=>(const Interval& a, const Interval& b) noexcept;

  explicit Interval(const Project& project);
  void set_project(const Project& project) noexcept;
  [[nodiscard]] const Project& project() const noexcept;
  void set_begin(const QDateTime& begin);
  void set_end(const QDateTime& end);
  [[nodiscard]] const QDateTime& begin() const noexcept;
  [[nodiscard]] const QDateTime& end() const noexcept;
  [[nodiscard]] QString duration_text() const;
  std::chrono::minutes duration() const;

private:
  const Project* m_project;
  QDateTime m_begin;
  QDateTime m_end;
};
