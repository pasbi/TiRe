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

class DeserializationError final : public RuntimeError
{
public:
  using RuntimeError::RuntimeError;
};

class InvalidEnumNameException final : public RuntimeError
{
public:
  using RuntimeError::RuntimeError;
};
