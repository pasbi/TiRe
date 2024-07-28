#pragma once
#include "json.h"

class Model;

[[nodiscard]] nlohmann::json serialize(const Model& model);
[[nodiscard]] std::unique_ptr<Model> deserialize(const nlohmann::json& json);