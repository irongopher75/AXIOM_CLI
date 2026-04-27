#pragma once

#include <string>
#include "core/domain.hpp"
#include "core/error.hpp"

namespace Axiom::Service {

class AnalysisService {
public:
    static Expected<AnalysisResult, DataError> analyze(const std::string& symbol);
    static Expected<MarkovResult, DataError> predict(const std::string& symbol);
    static bool fetch_and_cache(const std::string& symbol);
};

} // namespace Axiom::Service
