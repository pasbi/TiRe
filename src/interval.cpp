#include "interval.h"
#include "exceptions.h"
#include "json.h"
#include "period.h"

#include <nlohmann/json.hpp>

Interval::Interval(const Project& project) : m_project(&project)
{
}

const Project* Interval::swap_project(const Project* project) noexcept
{
  std::swap(project, m_project);
  return project;
}

[[nodiscard]] const Project& Interval::project() const noexcept
{
  return *m_project;
}

QDateTime Interval::swap_begin(QDateTime begin)
{
  swap(begin, m_begin);
  return begin;
}

QDateTime Interval::swap_end(QDateTime end)
{
  swap(end, m_end);
  return end;
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

std::chrono::minutes Interval::duration() const
{
  const auto end = this->end().isValid() ? this->end() : QDateTime::currentDateTime();
  using std::chrono_literals::operator""ms;
  return std::chrono::duration_cast<std::chrono::minutes>(begin().msecsTo(end) * 1ms);
}

Period Interval::period() const
{
  return Period{m_begin.date(), m_end.date()};
}

std::weak_ordering operator<=>(const Interval& a, const Interval& b) noexcept
{
  if (a.begin() == b.begin()) {
    return a.end() <=> b.end();
  }
  return a.begin() <=> b.begin();
}
