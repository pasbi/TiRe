#pragma once

#include <compare>
#include <cstdint>

/**
 * @class EntityId entityid.h "db/entityid.h"
 * @brief Opaque identity of a persisted entity.
 *
 * The id is a plain value and deliberately carries no dependency on Qt or SQL, so the domain
 * types that hold one stay usable (and unit-testable) without a database.
 *
 * Ids are assigned by the application rather than by the database: SQLite's `INTEGER PRIMARY KEY`
 * auto-assignment is rowid magic that PostgreSQL does not share, and `QSqlQuery::lastInsertId()`
 * is driver-dependent. Assigning up-front also means the id is known before the INSERT, which is
 * what lets an entity keep its original id when an undone removal is re-added.
 *
 * A default-constructed EntityId is invalid and denotes an entity that has never been persisted.
 */
class EntityId
{
public:
  using Value = std::int64_t;

  // Not explicit: Plan::Entry is an aggregate, and initializing it without naming the id
  // value-initializes this member, which an explicit default constructor would forbid.
  EntityId() noexcept = default;
  explicit EntityId(Value value) noexcept : m_value(value)
  {
  }

  [[nodiscard]] Value value() const noexcept
  {
    return m_value;
  }

  [[nodiscard]] bool is_valid() const noexcept
  {
    return m_value != 0;
  }

  friend auto operator<=>(const EntityId&, const EntityId&) noexcept = default;

private:
  Value m_value = 0;
};
