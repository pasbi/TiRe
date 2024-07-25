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

}  // namespace nlohmann
