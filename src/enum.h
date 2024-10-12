#pragma once

#include "exceptions.h"
#include "fmt.h"

template<typename EnumT, std::underlying_type_t<EnumT> size> EnumT enum_from_string(const std::string& s)
{
  for (std::underlying_type_t<EnumT> i = 0; i < size; ++i) {
    const auto e_val = static_cast<EnumT>(i);
    if (fmt::format("{}", e_val) == s) {
      return e_val;
    }
  }
  throw RuntimeError("Failed to parse Enum ({})", s);
}
