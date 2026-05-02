#include "analysis_service.hpp"
#include "engine/data_engine.hpp"
#include "engine/analysis_engine.hpp"
#include <iostream>

namespace Axiom::Service {

Expected<AnalysisResult, AxiomError> AnalysisService::analyze(const std::string& symbol) {
    // Service layer orchestrates the flow: Fetch -> Process
    return AnalysisEngine::run_analysis(symbol);
}

Expected<MarkovResult, AxiomError> AnalysisService::predict(const std::string& symbol) {
    return AnalysisEngine::run_markov(symbol);
}

bool AnalysisService::fetch_and_cache(const std::string& symbol) {
    auto res = DataEngine::fetch_prices(symbol, 1);
    return res.has_value();
}

} // namespace Axiom::Service
