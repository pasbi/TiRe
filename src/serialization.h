#pragma once
#include "json.h"

class TimeSheet;

[[nodiscard]] nlohmann::json serialize(const TimeSheet& time_sheet);
[[nodiscard]] std::unique_ptr<TimeSheet> deserialize(const nlohmann::json& json);