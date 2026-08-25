#pragma once

#include "period.h"
#include "plan.h"

class QString;

/**
 * @brief Stable, human-readable names under which enumerators are stored in the database.
 *
 * These are deliberately *not* the fmt formatters used for display: those go through
 * QObject::tr(), so they change with the user's language, and Period::Type::Custom has no display
 * label at all. Persistence names must never move.
 *
 * The implementations switch exhaustively with no default label, so adding an enumerator becomes
 * a compile error here rather than a silent data-loss bug.
 */
[[nodiscard]] QString db_name(Period::Type type);
[[nodiscard]] QString db_name(Plan::Kind kind);

/** @throws DatabaseError if @p name is not a known enumerator name. */
[[nodiscard]] Period::Type period_type_from_db_name(const QString& name);
[[nodiscard]] Plan::Kind plan_kind_from_db_name(const QString& name);
