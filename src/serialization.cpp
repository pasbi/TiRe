#include "serialization.h"
#include "exceptions.h"
#include "intervalmodel.h"
#include "plan.h"
#include "projectmodel.h"
#include "timesheet.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace
{
constexpr auto interval_model_key = "intervals";
constexpr auto project_model_key = "projects";
constexpr auto plan_key = "plan";
constexpr auto project_key = "project";
constexpr auto begin_key = "begin";
constexpr auto end_key = "end";

using ProjectIndexMap = std::map<const Project*, int>;

[[nodiscard]] auto create_project_index_map(const std::vector<Project*>& projects)
{
  ProjectIndexMap map;
  for (const auto* const project : projects) {
    map.try_emplace(project, static_cast<int>(map.size()));
  }
  return map;
}

[[nodiscard]] auto serialize(const IntervalModel& interval_model, const ProjectIndexMap& project_index_map)
{
  std::list<nlohmann::json> vs;
  for (const auto* const interval : interval_model.intervals()) {
    nlohmann::json& j = vs.emplace_back();
    j[begin_key] = interval->begin();
    j[end_key] = interval->end();
    try {
      j[project_key] = project_index_map.at(&interval->project());
    } catch (std::out_of_range&) {
      throw DeserializationError("Failed to store project reference.");
    }
  }
  return vs;
}

[[nodiscard]] auto serialize(const ProjectModel& project_model)
{
  std::list<nlohmann::json> vs;
  for (const auto* const project : project_model.projects()) {
    vs.emplace_back(project->to_json());
  }
  return vs;
}

[[nodiscard]] auto deserialize_interval_model(const nlohmann::json& data, const std::vector<Project*>& projects)
{
  std::deque<std::unique_ptr<Interval>> intervals;
  for (const auto& v : data) {
    try {
      const Project& project = *projects.at(v.at(project_key));
      auto& interval = *intervals.emplace_back(std::make_unique<Interval>(project));
      interval.set_begin(v.at(begin_key));
      interval.set_end(v.at(end_key));
    } catch (const std::out_of_range&) {
      throw DeserializationError("Failed to restore project reference.");
    }
  }
  return std::make_unique<IntervalModel>(std::move(intervals));
}

[[nodiscard]] auto deserialize_project_model(const nlohmann::json& data)
{
  std::vector<std::unique_ptr<Project>> projects;
  projects.reserve(data.size());
  for (const auto& v : data) {
    try {
      projects.emplace_back(std::make_unique<Project>(v));
    } catch (const InvalidEnumNameException& e) {
      throw DeserializationError("{}", e.what());
    }
  }
  try {
    return std::make_unique<ProjectModel>(std::move(projects));
  } catch (const std::runtime_error& e) {
    throw DeserializationError("{}", e.what());
  }
}

}  // namespace

nlohmann::json serialize(const TimeSheet& time_sheet)
{
  nlohmann::json j;
  j[project_model_key] = serialize(time_sheet.project_model());
  j[interval_model_key] =
      serialize(time_sheet.interval_model(), ::create_project_index_map(time_sheet.project_model().projects()));
  j[plan_key] = time_sheet.plan().to_json();
  return j;
}

std::unique_ptr<TimeSheet> deserialize(const nlohmann::json& json)
{
  try {
    auto project_model = ::deserialize_project_model(json.at(project_model_key));
    const auto projects = project_model->projects();
    auto interval_model = ::deserialize_interval_model(json.at(interval_model_key), projects);
    auto plan = std::make_unique<Plan>(json.at(plan_key));
    return std::make_unique<TimeSheet>(std::move(project_model), std::move(interval_model), std::move(plan));
  } catch (const nlohmann::json::out_of_range& e) {
    throw DeserializationError("Failed to load time sheet: {}", e.what());
  }
}