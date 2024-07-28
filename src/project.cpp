#include "project.h"
#include "exceptions.h"
#include "fmt.h"
#include "period.h"
#include <nlohmann/json.hpp>

namespace
{

constexpr auto name_key = "name";
constexpr auto type_key = "type";

[[nodiscard]] QString type_label(const Project::Type type)
{
  switch (type) {
    using enum Project::Type;
  case Work:
    return "Work";
  case Holiday:
    return "Holiday";
  case Sick:
    return "Sick";
  }
  return {};
}

[[nodiscard]] Project::Type label_type(const QString& name)
{
  using enum Project::Type;
  static constexpr auto types = std::array{Work, Holiday, Sick};
  const auto it = std::ranges::find_if(types, [name](const auto type) { return ::type_label(type) == name; });
  if (it != types.end()) {
    return *it;
  }
  throw InvalidEnumNameException("Failed to convert '{}' to Project::Type", name);
}

}  // namespace

Project::Project(const nlohmann::json& data) : m_name(data.at(name_key)), m_type(::label_type(data.at(type_key)))
{
}

Project::Project(Type type, QString name)
  : m_type(type), m_name(std::move(name))
{
}

const QString& Project::name() const noexcept
{
  return m_name;
}

Project::Type Project::type() const noexcept
{
  return m_type;
}

nlohmann::json Project::to_json() const
{
  return {
      {name_key, m_name},
      {type_key, ::type_label(m_type)},
  };
}

fmt::formatter<Project>::format_return_type fmt::formatter<Project>::format(const Project& p, fmt::format_context& ctx)
{
  if (p.type() == Project::Type::Work) {
    return fmt::format_to(ctx.out(), "Project[{}]", p.name().toStdString());
  }
  if (p.name().isEmpty()) {
    return fmt::format_to(ctx.out(), "Project({})", ::type_label(p.type()));
  }
  return fmt::format_to(ctx.out(), "Project({})[{}]", ::type_label(p.type()), p.name());
}

fmt::formatter<Project::Type>::format_return_type fmt::formatter<Project::Type>::format(const Project::Type& t,
                                                                                        fmt::format_context& ctx)
{
  return fmt::format_to(ctx.out(), "{}", ::type_label(t));
}

QString label(const Project* const project)
{
  if (project == nullptr) {
    return QObject::tr("No Project");
  }
  if (project->type() == Project::Type::Work) {
    return project->name();
  }
  return ::type_label(project->type());
}