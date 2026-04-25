#pragma once

#include <string>
#include <vector>
#include "../core/domain.hpp"
#include "../core/error.hpp"

namespace Axiom::DataEngine {

// ---------------------------------------------------------------------------
// Fetch `years` of daily close prices for `symbol` via the 3-tier engine:
//   1. Polygon.io  (primary, rate-limited)
//   2. Finnhub     (institutional backup, rate-limited)
//   3. Yahoo       (public fallback)
//
// Returns DataError::NetworkFailure if all three tiers fail.
// ---------------------------------------------------------------------------
Result<PriceData> fetch_prices(const std::string& symbol, int years);

// ---------------------------------------------------------------------------
// Search for ticker matches by name or symbol.  Tries Polygon first,
// falls back to Finnhub.  Always consumes rate-limiter tokens.
// ---------------------------------------------------------------------------
std::vector<SymbolMatch> resolve_symbol(const std::string& query);

} // namespace Axiom::DataEngine
