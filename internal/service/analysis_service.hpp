#pragma once

#include <string>
#include "core/domain.hpp"

namespace Axiom::Service {

class AnalysisService {
public:
    static AnalysisResult analyze(const std::string& symbol);
    static MarkovResult predict(const std::string& symbol);
    static bool fetch_and_cache(const std::string& symbol);
};

} // namespace Axiom::Service
