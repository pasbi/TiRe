#include "project.h"
#include "exceptions.h"
#include "fmt.h"
#include "period.h"
#include <nlohmann/json.hpp>

namespace
{

constexpr auto name_key = "name";
constexpr auto type_key = "type";
constexpr auto color_key = "color";

[[nodiscard]] QString type_label(const Project::Type type)
{
  switch (type) {
    using enum Project::Type;
  case Empty:
    return "Empty";
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
  static constexpr auto types = std::array{Empty, Work, Holiday, Sick};
  if (const auto it = std::ranges::find_if(types, [name](const auto type) { return ::type_label(type) == name; });
      it != types.end())
  {
    return *it;
  }
  throw InvalidEnumNameException("Failed to convert '{}' to Project::Type", name);
}

}  // namespace

Project::Project(const nlohmann::json& data)
  : m_type(::label_type(data.at(type_key)))
  , m_name(data.at(name_key))
  , m_color(QColor::fromString(data.value(color_key, QString{})))
{
}

Project::Project(const Type type, QString name, const QColor& color)
  : m_type(type), m_name(std::move(name)), m_color(color)
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
      {color_key, m_color.name()},
  };
}
QString Project::label() const
{
  switch (m_type) {
    using enum Type;
  case Empty:
    return QObject::tr("No Project");
  case Work:
    return name();
  default:
    return ::type_label(m_type);
  }
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
