#pragma once

#include <QDate>
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

template<> struct fmt::formatter<QDateTime> : fmt::formatter<std::string>
{
  [[nodiscard]] static auto format(const QDateTime& t, fmt::format_context& ctx)
  {
    return fmt::format_to(ctx.out(), "{}", t.toString(Qt::ISODate));
  }
};
