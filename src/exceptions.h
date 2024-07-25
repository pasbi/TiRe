#include <format>
#include <stdexcept>

class RuntimeError : public std::runtime_error
{
public:
  template<typename... Args> explicit RuntimeError(std::format_string<Args...> format_string, Args&&... args)
    : std::runtime_error(std::format(std::move(format_string), std::forward<Args>(args)...))
  {
  }
};

class DeserializationError : public RuntimeError
{
public:
  using RuntimeError::RuntimeError;
};
