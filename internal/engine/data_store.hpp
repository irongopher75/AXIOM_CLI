#pragma once
#include <vector>
#include <string>
#include "core/domain.hpp"

namespace Axiom::DataEngine {

class DataStore {
public:
    virtual ~DataStore() = default;
    
    // Abstract interface for persistence
    virtual void write_bars(const std::string& symbol, const std::vector<Bar>& bars, const std::string& source, const std::string& fetched_at) = 0;
    virtual std::vector<Bar> query_bars(const std::string& symbol, const std::string& fetched_at) = 0;
    virtual void flush() = 0;
};

// Global singleton access
DataStore* get_store();
void init_store();
void shutdown_store();

} // namespace Axiom::DataEngine
