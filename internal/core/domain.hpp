#pragma once

#include <string>
#include <vector>
#include <map>
#include "types.hpp"

namespace Axiom {

// ---------------------------------------------------------------------------
// A single market bar (candlestick).
// ---------------------------------------------------------------------------
struct Bar {
    Price   o, h, l, c;
    int64_t v; // Volume
};

// ---------------------------------------------------------------------------
// Raw data from the data engine. No UI concern touches this struct.
// ---------------------------------------------------------------------------
struct PriceData {
    std::vector<Bar>   bars;
    std::string        source;
    std::string        exchange;
    std::string        country;
};

// ---------------------------------------------------------------------------
// Output of run_analysis(). Stores raw prices so the REPL can render the
// chart itself — keeping UI::get_chart() out of the analysis hot path.
// ---------------------------------------------------------------------------
struct AnalysisResult {
    std::string        symbol;
    std::string        country;   // metadata
    std::string        exchange;  // metadata
    Price              price     { 0.0 };
    double             change    { 0.0 };
    Price              sma50     { 0.0 };
    double             rsi       { 50.0 };
    
    // --- Core Advanced Indicators ---
    Price              atr14     { 0.0 };
    Price              bb_upper  { 0.0 };
    Price              bb_lower  { 0.0 };
    Price              vwap      { 0.0 };
    double             mfi       { 50.0 };
    double             adx       { 0.0 };
    double             hurst     { 0.5 };
    
    // --- All 50+ Indicators ---
    std::map<std::string, double> stats;

    std::vector<Bar>   history;   // raw, for chart rendering at display time
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
    std::vector<std::vector<double>> transition_matrix; // for heatmap UI
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
    std::string country;
};

} // namespace Axiom
