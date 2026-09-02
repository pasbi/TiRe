#pragma once

#include "fmt.h"
#include <stdexcept>

class RuntimeError : public std::runtime_error
{
public:
  template<typename... Args> explicit RuntimeError(fmt::format_string<Args...> format_string, Args&&... args)
    : std::runtime_error(fmt::format(std::move(format_string), std::forward<Args>(args)...))
  {
  }
};

/**
 * @brief Thrown when the database cannot be opened, migrated, read or written.
 * Derives from RuntimeError so call sites that already report a RuntimeError to the user
 * (e.g. PlanTableView::open_period_edit) handle persistence failures without change.
 */
class DatabaseError final : public RuntimeError
{
public:
  using RuntimeError::RuntimeError;
};
