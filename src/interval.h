#pragma once

#include <QDateTime>
#include <compare>
#include <nlohmann/adl_serializer.hpp>

class Project;
class Period;

class Interval
{
public:
  friend std::weak_ordering operator<=>(const Interval& a, const Interval& b) noexcept;

  explicit Interval(const Project& project);
  const Project* swap_project(const Project* project) noexcept;
  [[nodiscard]] const Project& project() const noexcept;
  QDateTime swap_begin(QDateTime begin);
  QDateTime swap_end(QDateTime end);
  [[nodiscard]] const QDateTime& begin() const noexcept;
  [[nodiscard]] const QDateTime& end() const noexcept;
  [[nodiscard]] QString duration_text() const;
  std::chrono::minutes duration() const;
  Period period() const;

private:
  const Project* m_project;
  QDateTime m_begin;
  QDateTime m_end;
};
