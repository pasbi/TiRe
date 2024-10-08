#pragma once
#include "fmt.h"
#include "json.h"

#include <QColor>

class Project
{
public:
  explicit Project(const nlohmann::json& data);
  explicit Project(QString name, const QColor& color);
  explicit Project() = default;

  [[nodiscard]] const QString& name() const noexcept;
  [[nodiscard]] nlohmann::json to_json() const;

  [[nodiscard]] const QColor& color() const noexcept;
  void set_color(const QColor& color) noexcept;

private:
  QString m_name;
  QColor m_color;
};

template<> struct fmt::formatter<Project> : fmt::formatter<std::string>
{
  using format_return_type = decltype(std::declval<format_context>().out());
  [[nodiscard]] static format_return_type format(const Project& p, fmt::format_context& ctx);
};
