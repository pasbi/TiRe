#include "json.h"

#include <QDateTime>
#include <nlohmann/json.hpp>

namespace nlohmann
{

void adl_serializer<QString>::to_json(json& j, const QString& value)
{
  j = value.toStdString();
}

void adl_serializer<QString>::from_json(const json& j, QString& value)
{
  value = QString::fromStdString(static_cast<std::string>(j));
}

void adl_serializer<QDateTime>::to_json(json& j, const QDateTime& value)
{
  j = value.toString(Qt::ISODate);
}

void adl_serializer<QDateTime>::from_json(const json& j, QDateTime& value)
{
  value = QDateTime::fromString(j.get<QString>(), Qt::ISODate);
}

void adl_serializer<QList<QString>, void>::to_json(json& j, const QStringList& value)
{
  j = std::vector(value.begin(), value.end());
}

void adl_serializer<QList<QString>, void>::from_json(const json& j, QStringList& value)
{
  auto vec = j.get<std::vector<QString>>();
  value = QStringList(vec.begin(), vec.end());
}

}  // namespace nlohmann
