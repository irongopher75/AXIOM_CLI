#pragma once

#include <string>
#include <vector>
#include "types.hpp"

namespace Axiom {

// ---------------------------------------------------------------------------
// Raw data from the data engine. No UI concern touches this struct.
// ---------------------------------------------------------------------------
struct PriceData {
    std::vector<Price> prices;
    std::string        source;
};

// ---------------------------------------------------------------------------
// Output of run_analysis(). Stores raw prices so the REPL can render the
// chart itself — keeping UI::get_chart() out of the analysis hot path.
// ---------------------------------------------------------------------------
struct AnalysisResult {
    std::string        symbol;
    Price              price     { 0.0 };
    double             change    { 0.0 };
    Price              sma50     { 0.0 };
    double             rsi       { 50.0 };
    std::vector<Price> history;   // raw, for chart rendering at display time
    std::string        sentiment;
    std::string        source;
    std::string        timestamp;
    bool               ok        { true };
};

// ---------------------------------------------------------------------------
// Output of run_markov().
// ---------------------------------------------------------------------------
struct MarkovResult {
    std::string symbol;
    std::string prediction;
    double      confidence  { 0.0 };
    double      aic         { 0.0 };
    bool        aic_valid   { false };
    std::string source;
    std::string timestamp;
    bool        ok          { true };
};

// ---------------------------------------------------------------------------
// A single ticker search result.
// ---------------------------------------------------------------------------
struct SymbolMatch {
    std::string symbol;
    std::string name;
    std::string exchange;
};

} // namespace Axiom
