#pragma once

#include <QDateTime>
#include <QString>
#include <nlohmann/json_fwd.hpp>

namespace nlohmann
{
template<> struct adl_serializer<QString>
{
  static void to_json(json& j, const QString& value);
  static void from_json(const json& j, QString& value);
};

template<> struct adl_serializer<std::chrono::minutes>
{
  static void to_json(json& j, const std::chrono::minutes& value);
  static void from_json(const json& j, std::chrono::minutes& value);
};

template<> struct adl_serializer<QDate>
{
  static void to_json(json& j, const QDate& value);
  static void from_json(const json& j, QDate& value);
};

template<> struct adl_serializer<QDateTime>
{
  static void to_json(json& j, const QDateTime& value);
  static void from_json(const json& j, QDateTime& value);
};

template<> struct adl_serializer<QStringList>
{
  static void to_json(json& j, const QStringList& value);
  static void from_json(const json& j, QStringList& value);
};

}  // namespace nlohmann
