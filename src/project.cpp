#include "project.h"
#include "exceptions.h"
#include "fmt.h"
#include "period.h"
#include <nlohmann/json.hpp>

namespace
{

constexpr auto name_key = "name";
constexpr auto color_key = "color";

}  // namespace

Project::Project(const nlohmann::json& data)
  : m_name(data.at(name_key)), m_color(QColor::fromString(data.value(color_key, QString{})))
{
}

Project::Project(QString name, const QColor& color) : m_name(std::move(name)), m_color(color)
{
}

const QString& Project::name() const noexcept
{
  return m_name;
}

nlohmann::json Project::to_json() const
{
  return {
      {name_key, m_name},
      {color_key, m_color.name()},
  };
}

const QColor& Project::color() const noexcept
{
  return m_color;
}

void Project::set_color(const QColor& color) noexcept
{
  m_color = color;
}

fmt::formatter<Project>::format_return_type fmt::formatter<Project>::format(const Project& p, fmt::format_context& ctx)
{
  return fmt::format_to(ctx.out(), "Project[{}]", p.name().toStdString());
}
