#pragma once

#include <string>
#include "core/domain.hpp"
#include "core/error.hpp"

namespace Axiom::AnalysisEngine {

// ---------------------------------------------------------------------------
// Technical analysis: SMA-50, RSI-14, sentiment classification.
// Stores raw price history in the result so the caller controls rendering.
//
// Fetches 1 year of daily data via DataEngine::fetch_prices().
// ---------------------------------------------------------------------------
Expected<AnalysisResult, DataError> run_analysis(const std::string& symbol);

// ---------------------------------------------------------------------------
// Markov chain regime predictor.
// Estimates a 3-state (Bearish/Neutral/Bullish) transition matrix from
// 2 years of daily returns using MLE, then validates the model against a
// random-walk null via AIC.
//
// Fetches 2 years of daily data via DataEngine::fetch_prices().
// ---------------------------------------------------------------------------
Expected<MarkovResult, DataError> run_markov(const std::string& symbol);

} // namespace Axiom::AnalysisEngine
