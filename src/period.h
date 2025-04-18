#pragma once

#include <QDate>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

class Interval;

/**
 * @class Period period.h "period.h"
 * @brief A Period represents a time range with well-defined begin and end date.
 * At first glance, it resembles the Interval, however, its purpose is very different.
 * - The Period usually reflects a common time range (e.g., last week, yesterday, etc.).
 * - The Period has a precision of a day, while Interval is precise up to a minute.
 * - The Period must have both begin and end, the interval may not have an end if it is ongoing.
 * @see Interval
 */
class Period
{
public:
  enum class Type { Year, Month, Week, Day, Custom };
  explicit Period(const QDate& date, Type type);
  explicit Period(const QDate& begin, const QDate& end);
  explicit Period() = default;
  [[nodiscard]] const QDate& begin() const noexcept;
  [[nodiscard]] const QDate& end() const noexcept;
  [[nodiscard]] Type type() const noexcept;
  [[nodiscard]] std::chrono::minutes overlap(const Interval& interval) const noexcept;
  [[nodiscard]] std::optional<Period> overlap(const Period& period) const noexcept;
  [[nodiscard]] QString label() const;
  [[nodiscard]] bool contains(const Period& period) const noexcept;
  [[nodiscard]] bool contains(const QDate& date) const noexcept;
  [[nodiscard]] int days() const noexcept;
  [[nodiscard]] QDate clamp(const QDate& date) const noexcept;
  [[nodiscard]] QDateTime clamp(const QDateTime& date_time) const noexcept;
  [[nodiscard]] Period constrained(const QDate& latest_begin, const QDate& earliest_end) const;
  [[nodiscard]] std::pair<QDate, QDate> limits() const noexcept;
  [[nodiscard]] std::vector<QDate> dates() const;

private:
  QDate m_begin;
  QDate m_end;
  Type m_type = Type::Custom;

  friend std::weak_ordering operator<=>(const Period& a, const Period& b) noexcept;
  friend bool operator==(const Period&, const Period&) noexcept = default;
  friend bool operator!=(const Period&, const Period&) noexcept = default;
  friend void swap(Period& a, Period& b);
};

template<> struct fmt::formatter<Period> : fmt::formatter<std::string>
{
  using format_return_type = decltype(std::declval<format_context>().out());
  [[nodiscard]] static format_return_type format(const Period& p, fmt::format_context& ctx);
};

template<> struct fmt::formatter<Period::Type> : fmt::formatter<std::string>
{
  using format_return_type = decltype(std::declval<format_context>().out());
  [[nodiscard]] format_return_type format(const Period::Type& t, fmt::format_context& ctx) const;
};

void to_json(nlohmann::json& j, const Period& value);
void from_json(const nlohmann::json& j, Period& value);
void to_json(nlohmann::json& j, const Period::Type& value);
void from_json(const nlohmann::json& j, Period::Type& value);
