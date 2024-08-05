#pragma once

#include <QDate>
#include <fmt/format.h>

class Interval;

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
  [[nodiscard]] QString label() const;
  [[nodiscard]] bool contains(const Period& period) const noexcept;
  [[nodiscard]] int days() const noexcept;

private:
  QDate m_begin;
  QDate m_end;
  Type m_type;

  friend std::weak_ordering operator<=>(const Period& a, const Period& b) noexcept;
  friend bool operator==(const Period&, const Period&) noexcept = default;
  friend bool operator!=(const Period&, const Period&) noexcept = default;
};

template<> struct fmt::formatter<Period> : fmt::formatter<std::string>
{
  using format_return_type = decltype(std::declval<format_context>().out());
  [[nodiscard]] static format_return_type format(const Period& p, fmt::format_context& ctx);
};

template<> struct fmt::formatter<Period::Type> : fmt::formatter<std::string>
{
  using format_return_type = decltype(std::declval<format_context>().out());
  [[nodiscard]] format_return_type format(const Period::Type& t, fmt::format_context& ctx);
};
