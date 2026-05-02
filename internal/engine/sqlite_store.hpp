#pragma once
#include "data_store.hpp"
#include <sqlite3.h>

namespace Axiom::DataEngine {

class SQLiteStore : public DataStore {
private:
    sqlite3* db;

public:
    SQLiteStore(const std::string& db_path);
    ~SQLiteStore() override;

    void write_bars(const std::string& symbol, const std::vector<Bar>& bars, const std::string& source, const std::string& fetched_at) override;
    std::vector<Bar> query_bars(const std::string& symbol, const std::string& fetched_at) override;
    void flush() override;
};

} // namespace Axiom::DataEngine
