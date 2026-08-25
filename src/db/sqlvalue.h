#pragma once

#include <chrono>

class QColor;
class QDate;
class QDateTime;
class QString;
class QVariant;

/**
 * @brief Conversions between domain values and the QVariants bound to / read from SQL.
 *
 * Everything is stored in types both SQLite and PostgreSQL share, so the schema survives a
 * driver swap:
 * - QDateTime / QDate become fixed-width ISO-8601 TEXT. Fixed width matters: it makes a
 *   lexicographic `BETWEEN` on begin_time a correct chronological range scan.
 * - QColor becomes '#aarrggbb' TEXT. Note this keeps the alpha channel, which the previous
 *   JSON format silently dropped by using QColor::name().
 * - std::chrono::minutes becomes a plain integer count, which may be negative
 *   (Plan::overtime_offset can be).
 *
 * An invalid QDateTime / QDate maps to a null QVariant and back. That round-trip is what
 * expresses "this interval is still running" (interval.end_time IS NULL).
 */
[[nodiscard]] QVariant to_sql(const QDateTime& date_time);
[[nodiscard]] QVariant to_sql(const QDate& date);
[[nodiscard]] QVariant to_sql(const QColor& color);
[[nodiscard]] QVariant to_sql(std::chrono::minutes minutes);

[[nodiscard]] QDateTime date_time_from_sql(const QVariant& value);
[[nodiscard]] QDate date_from_sql(const QVariant& value);
[[nodiscard]] QColor color_from_sql(const QVariant& value);
[[nodiscard]] std::chrono::minutes minutes_from_sql(const QVariant& value);
