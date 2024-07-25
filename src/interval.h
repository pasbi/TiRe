#pragma once

#include <QDateTime>
#include <compare>
#include <nlohmann/json_fwd.hpp>

class Project;

class Interval
{
public:
  friend std::weak_ordering operator<=>(const Interval& a, const Interval& b) noexcept;

  void set_project(QString project) noexcept;
  [[nodiscard]] const QString& project() const noexcept;
  void set_begin(const QDateTime& begin);
  void set_end(const QDateTime& end);
  [[nodiscard]] const QDateTime& begin() const noexcept;
  [[nodiscard]] const QDateTime& end() const noexcept;
  [[nodiscard]] QString duration_text() const;

private:
  QString m_project;
  QDateTime m_begin;
  QDateTime m_end;
};

namespace nlohmann
{
template<> struct adl_serializer<Interval>
{
  static void to_json(json& j, const Interval& value);
  static void from_json(const json& j, Interval& value);
};

}  // namespace nlohmann
