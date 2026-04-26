#include "analysis_service.hpp"
#include "engine/data_engine.hpp"
#include "engine/analysis_engine.hpp"
#include <iostream>

namespace Axiom::Service {

AnalysisResult AnalysisService::analyze(const std::string& symbol) {
    // Service layer orchestrates the flow: Fetch -> Process
    auto res = AnalysisEngine::run_analysis(symbol);
    return res;
}

MarkovResult AnalysisService::predict(const std::string& symbol) {
    auto res = AnalysisEngine::run_markov(symbol);
    return res;
}

bool AnalysisService::fetch_and_cache(const std::string& symbol) {
    auto res = DataEngine::fetch_prices(symbol, 1);
    return res.has_value();
}

} // namespace Axiom::Service
