#include "interval.h"
#include "exceptions.h"
#include "json.h"

#include <nlohmann/json.hpp>

Interval::Interval(const Project& project) : m_project(&project)
{
}

void Interval::set_project(const Project& project) noexcept
{
  m_project = &project;
}

[[nodiscard]] const Project& Interval::project() const noexcept
{
  return *m_project;
}

void Interval::set_begin(const QDateTime& begin)
{
  m_begin = begin;
}

void Interval::set_end(const QDateTime& end)
{
  m_end = end;
}

[[nodiscard]] const QDateTime& Interval::begin() const noexcept
{
  return m_begin;
}

[[nodiscard]] const QDateTime& Interval::end() const noexcept
{
  return m_end;
}

QString Interval::duration_text() const
{
  if (m_begin.isNull()) {
    return {};
  }
  const auto duration = (m_end.isNull() ? QDateTime::currentDateTime() : m_end) - m_begin;
  using std::chrono_literals::operator""min;
  const auto duration_min = duration / 1min;
  const auto minutes = duration_min % 60;
  const auto hours = duration_min / 60;
  static constexpr auto field_width = 2;
  static constexpr auto base = 10;
  static constexpr auto fill_char = QChar('0');
  return QStringLiteral("%1:%2%3")
      .arg(hours, field_width, base, fill_char)
      .arg(minutes, field_width, base, fill_char)
      .arg(m_end.isNull() ? "*" : "");
}

std::weak_ordering operator<=>(const Interval& a, const Interval& b) noexcept
{
  if (a.begin() == b.begin()) {
    return a.end() <=> b.end();
  }
  return a.begin() <=> b.begin();
}
