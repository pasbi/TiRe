#include "project.h"
#include "exceptions.h"
#include "fmt.h"
#include "period.h"

Project::Project(QString name, const QColor& color) : m_name(std::move(name)), m_color(color)
{
}

const QString& Project::name() const noexcept
{
  return m_name;
}

const QColor& Project::color() const noexcept
{
  return m_color;
}

void Project::set_color(const QColor& color) noexcept
{
  m_color = color;
}

EntityId Project::id() const noexcept
{
  return m_id;
}

void Project::set_id(const EntityId id) noexcept
{
  m_id = id;
}

fmt::formatter<Project>::format_return_type fmt::formatter<Project>::format(const Project& p, fmt::format_context& ctx)
{
  return fmt::format_to(ctx.out(), "Project[{}]", p.name().toStdString());
}
