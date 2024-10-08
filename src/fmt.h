#pragma once

#include <QColor>
#include <QDate>
#include <QPointF>
#include <QString>
#include <fmt/format.h>

template<> struct fmt::formatter<QString> : fmt::formatter<std::string>
{
  [[nodiscard]] static auto format(const QString& t, fmt::format_context& ctx)
  {
    return fmt::format_to(ctx.out(), "{}", t.toStdString());
  }
};

template<> struct fmt::formatter<QDate> : fmt::formatter<std::string>
{
  [[nodiscard]] static auto format(const QDate& t, fmt::format_context& ctx)
  {
    return fmt::format_to(ctx.out(), "{}", t.toString(Qt::ISODate));
  }
};

template<> struct fmt::formatter<QTime> : fmt::formatter<std::string>
{
  [[nodiscard]] static auto format(const QTime& t, fmt::format_context& ctx)
  {
    return fmt::format_to(ctx.out(), "{}", t.toString(Qt::ISODate));
  }
};

template<> struct fmt::formatter<QDateTime> : fmt::formatter<std::string>
{
  [[nodiscard]] static auto format(const QDateTime& t, fmt::format_context& ctx)
  {
    return fmt::format_to(ctx.out(), "{}", t.toString(Qt::ISODate));
  }
};

template<> struct fmt::formatter<QPointF> : fmt::formatter<std::string>
{
  [[nodiscard]] static auto format(const QPointF& p, fmt::format_context& ctx)
  {
    return fmt::format_to(ctx.out(), "[{}, {}]", p.x(), p.y());
  }
};

template<> struct fmt::formatter<QColor> : fmt::formatter<std::string>
{
  [[nodiscard]] static auto format(const QColor& c, fmt::format_context& ctx)
  {
    return fmt::format_to(ctx.out(), "{}", c.name());
  }
};

template<> struct fmt::formatter<std::chrono::minutes> : fmt::formatter<std::string>
{
  [[nodiscard]] static auto format(std::chrono::minutes min, fmt::format_context& ctx)
  {
    using std::chrono_literals::operator""min;
    return fmt::format_to(ctx.out(), "{}min", min / 1min);
  }
};
