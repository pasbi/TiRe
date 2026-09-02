#pragma once
#include "db/entityid.h"
#include "fmt.h"

#include <QColor>

class Project
{
public:
  explicit Project(QString name, const QColor& color);
  explicit Project() = default;

  [[nodiscard]] const QString& name() const noexcept;

  [[nodiscard]] const QColor& color() const noexcept;
  void set_color(const QColor& color) noexcept;

  /**
   * @brief Identity of this project's row, invalid until it has been persisted.
   * The id travels with the object so that a project removed by a command and put back by undo
   * returns to its original row, keeping the intervals that reference it valid.
   */
  [[nodiscard]] EntityId id() const noexcept;
  void set_id(EntityId id) noexcept;

private:
  QString m_name;
  QColor m_color;
  EntityId m_id;
};

template<> struct fmt::formatter<Project> : fmt::formatter<std::string>
{
  using format_return_type = decltype(std::declval<format_context>().out());
  [[nodiscard]] static format_return_type format(const Project& p, fmt::format_context& ctx);
};
