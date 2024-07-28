#pragma once
#include "fmt.h"
#include "json.h"

class Project
{
public:
  explicit Project(const nlohmann::json& data);
  enum class Type { Work, Holiday, Sick };

  [[nodiscard]] const QString& name() const noexcept;
  [[nodiscard]] Type type() const noexcept;
  [[nodiscard]] nlohmann::json to_json() const;

private:
  QString m_name;
  Type m_type;
};

template<> struct fmt::formatter<Project> : fmt::formatter<std::string>
{
  using format_return_type = decltype(std::declval<format_context>().out());
  [[nodiscard]] static format_return_type format(const Project& p, fmt::format_context& ctx);
};

template<> struct fmt::formatter<Project::Type> : fmt::formatter<std::string>
{
  using format_return_type = decltype(std::declval<format_context>().out());
  [[nodiscard]] format_return_type format(const Project::Type& t, fmt::format_context& ctx);
};

[[nodiscard]] QString label(const Project* project);
