#pragma once

#include <span>
#include <string_view>

class Database;

/**
 * @struct Migration migrations.h "db/migrations.h"
 * @brief One forward step of the database schema.
 *
 * Migrations are applied in ascending version order and each one runs in its own transaction, so
 * an interrupted upgrade leaves the database at the last version that fully succeeded.
 */
struct Migration
{
  int version;
  std::string_view description;
  void (*apply)(Database& database);
};

/** @brief All known migrations, sorted ascending by version. */
[[nodiscard]] std::span<const Migration> migrations();

/** @brief The version a fully migrated database ends up at. */
[[nodiscard]] int latest_schema_version();
