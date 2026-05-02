#include "sqlite_store.hpp"
#include "utils/logger.hpp"
#include <iostream>

namespace Axiom::DataEngine {

SQLiteStore::SQLiteStore(const std::string& db_path) : db(nullptr) {
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        Utils::Logger::error("Failed to open SQLite database: " + std::string(sqlite3_errmsg(db)));
        db = nullptr;
        return;
    }

    // Enable WAL mode for high throughput
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);

    const char* schema = R"(
        CREATE TABLE IF NOT EXISTS bars (
            symbol TEXT,
            idx INTEGER,
            open DOUBLE,
            high DOUBLE,
            low DOUBLE,
            close DOUBLE,
            volume BIGINT,
            source TEXT,
            fetched_at TEXT
        );
        CREATE INDEX IF NOT EXISTS idx_bars_symbol ON bars(symbol, fetched_at);
    )";

    char* err_msg = nullptr;
    if (sqlite3_exec(db, schema, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        Utils::Logger::error("Failed to initialize SQLite schema: " + std::string(err_msg));
        sqlite3_free(err_msg);
    }
}

SQLiteStore::~SQLiteStore() {
    if (db) {
        sqlite3_close(db);
    }
}

void SQLiteStore::write_bars(const std::string& symbol, const std::vector<Bar>& bars, const std::string& source, const std::string& fetched_at) {
    if (!db) return;

    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    // Delete existing cache for this symbol to avoid duplicates if re-fetched today
    std::string del_sql = "DELETE FROM bars WHERE symbol = ?;";
    sqlite3_stmt* stmt_del;
    if (sqlite3_prepare_v2(db, del_sql.c_str(), -1, &stmt_del, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt_del, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt_del);
        sqlite3_finalize(stmt_del);
    }

    const char* ins_sql = "INSERT INTO bars (symbol, idx, open, high, low, close, volume, source, fetched_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, ins_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        Utils::Logger::error("Failed to prepare insert statement");
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return;
    }

    for (size_t i = 0; i < bars.size(); ++i) {
        const auto& b = bars[i];
        sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 2, i);
        sqlite3_bind_double(stmt, 3, b.o.to_double());
        sqlite3_bind_double(stmt, 4, b.h.to_double());
        sqlite3_bind_double(stmt, 5, b.l.to_double());
        sqlite3_bind_double(stmt, 6, b.c.to_double());
        sqlite3_bind_int64(stmt, 7, b.v);
        sqlite3_bind_text(stmt, 8, source.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 9, fetched_at.c_str(), -1, SQLITE_TRANSIENT);

        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
}

std::vector<Bar> SQLiteStore::query_bars(const std::string& symbol, const std::string& fetched_at) {
    std::vector<Bar> bars;
    if (!db) return bars;

    const char* sql = "SELECT open, high, low, close, volume FROM bars WHERE symbol = ? AND fetched_at = ? ORDER BY idx ASC;";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return bars;
    }

    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, fetched_at.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        bars.push_back({
            Price(sqlite3_column_double(stmt, 0)),
            Price(sqlite3_column_double(stmt, 1)),
            Price(sqlite3_column_double(stmt, 2)),
            Price(sqlite3_column_double(stmt, 3)),
            (int64_t)sqlite3_column_int64(stmt, 4)
        });
    }

    sqlite3_finalize(stmt);
    return bars;
}

void SQLiteStore::flush() {
    if (db) {
        // WAL checkpoint
        sqlite3_wal_checkpoint_v2(db, "main", SQLITE_CHECKPOINT_FULL, nullptr, nullptr);
    }
}

// ---------------------------------------------------------------------------
// Singleton Access
// ---------------------------------------------------------------------------

static std::unique_ptr<DataStore> global_store = nullptr;

DataStore* get_store() {
    return global_store.get();
}

void init_store() {
    if (!global_store) {
        std::string db_path = Axiom::Paths::get_path("axiom.sqlite");
        global_store = std::make_unique<SQLiteStore>(db_path);
    }
}

void shutdown_store() {
    if (global_store) {
        global_store->flush();
        global_store.reset();
    }
}

} // namespace Axiom::DataEngine
