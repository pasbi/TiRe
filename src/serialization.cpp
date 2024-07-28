#include "serialization.h"

#include "exceptions.h"
#include "json.h"
#include "model.h"

#include <nlohmann/json.hpp>

namespace
{
constexpr auto intervals_key = "intervals";
constexpr auto projects_key = "projects";
constexpr auto project_key = "project";
constexpr auto begin_key = "begin";
constexpr auto end_key = "end";

template<typename ProjectPointer>
[[nodiscard]] nlohmann::json serialize_projects(const std::vector<ProjectPointer>& projects)
{
  auto view = projects | std::views::transform(&Project::to_json);
  return std::vector(view.begin(), view.end());
}

std::vector<std::unique_ptr<Project>> deserialize_projects(const nlohmann::json& json)
{
  std::vector<std::unique_ptr<Project>> projects;
  projects.reserve(json.size());
  for (const auto& data : json) {
    projects.emplace_back(std::make_unique<Project>(json));
  }
  return projects;
}

std::deque<Interval> deserialize_intervals(const nlohmann::json& json, const std::vector<Project*>& projects)
{
  std::deque<Interval> intervals;
  for (const auto& data : json) {
    auto& interval = intervals.emplace_back();
    try {
      interval.set_project(projects.at(data[project_key]));
    } catch (const std::out_of_range&) {
      throw DeserializationError("Failed to restore project reference.");
    }
    interval.set_begin(data[begin_key]);
    interval.set_end(data[end_key]);
  }
  return intervals;
}

}  // namespace

nlohmann::json serialize(const Model& model)
{
  std::map<const Project*, int> project_index_map;
  for (const auto& project : model.projects()) {
    project_index_map.try_emplace(project, static_cast<int>(project_index_map.size()));
  }

  const auto serialize_interval = [&project_index_map](const Interval& interval) {
    return nlohmann::json{
        {project_key, project_index_map.at(interval.project())},
        {begin_key, interval.begin()},
        {end_key, interval.end()},
    };
  };
  auto project_indices = model.intervals() | std::views::transform(serialize_interval);
  return {
      {::intervals_key, std::vector(project_indices.begin(), project_indices.end())},
      {::projects_key, ::serialize_projects(model.projects())},
  };
}

std::unique_ptr<Model> deserialize(const nlohmann::json& json)
{
  try {
    auto projects = ::deserialize_projects(json.at(projects_key));
    auto projects_view = projects | std::views::transform(&std::unique_ptr<Project>::get);
    auto intervals =
        ::deserialize_intervals(json.at(intervals_key), std::vector(projects_view.begin(), projects_view.end()));

    return std::make_unique<Model>(std::move(projects), std::move(intervals));
  } catch (const nlohmann::json::out_of_range& e) {
    throw DeserializationError("Failed to load project: {}", e.what());
  }
}