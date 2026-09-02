#pragma once

#include "db/abstracttimesheetrepository.h"
#include "db/entityid.h"

#include <memory>

class Database;
class TimeSheet;

/**
 * @class SqlTimeSheetRepository sqltimesheetrepository.h "db/sqltimesheetrepository.h"
 * @brief Reads and writes the timesheet through a SQL Database.
 *
 * Ids are handed out by this class rather than by the database, so that they are known before
 * the INSERT and stay portable across SQL dialects. The counters are seeded from the highest id
 * already stored, which happens in the constructor so that a repository used without load() is
 * equally safe.
 */
class SqlTimeSheetRepository final : public AbstractTimeSheetRepository
{
public:
  explicit SqlTimeSheetRepository(Database& database);

  /**
   * @brief Reads the whole timesheet into memory.
   * Creates the plan settings row if this is a fresh database.
   * @throws DatabaseError if the stored data cannot be interpreted.
   */
  [[nodiscard]] std::unique_ptr<TimeSheet> load();

  void insert(Project& project) override;
  void update(const Project& project) override;
  void remove(const Project& project) override;

  void insert(Interval& interval) override;
  void update(const Interval& interval) override;
  void remove(const Interval& interval) override;

  void insert(Plan::Entry& entry) override;
  void update(const Plan::Entry& entry) override;
  void remove(const Plan::Entry& entry) override;

  void update_plan_setting(const Plan& plan) override;

private:
  [[nodiscard]] static EntityId next_id(EntityId::Value& counter) noexcept;
  void seed_id_counters();

  Database& m_database;
  EntityId::Value m_next_project_id = 0;
  EntityId::Value m_next_interval_id = 0;
  EntityId::Value m_next_plan_entry_id = 0;
};
