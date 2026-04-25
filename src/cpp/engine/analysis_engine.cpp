#include "analysis_engine.hpp"
#include "data_engine.hpp"

#include <cmath>
#include <ctime>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include "../core/buffers.hpp"

// ---------------------------------------------------------------------------
// Internal: ISO timestamp string for result metadata
// ---------------------------------------------------------------------------
static std::string now_str() {
    auto t  = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

// ===========================================================================
// Public API
// ===========================================================================

namespace Axiom::AnalysisEngine {

AnalysisResult run_analysis(const std::string& symbol) {
    AnalysisResult res;
    res.symbol = symbol;

    auto fetched = DataEngine::fetch_prices(symbol, 1);
    if (!fetched.has_value()) {
        res.ok        = false;
        res.sentiment = "Error";
        return res;
    }

    auto& data = fetched.value();
    res.source  = data.source;
    res.history = data.prices;   // caller renders the chart

    if (data.prices.empty()) {
        res.ok        = false;
        res.sentiment = "Error";
        return res;
    }

    // --- Current price & daily change ---
    res.price   = data.prices.back();
    Price prev  = data.prices.size() > 1
                ? data.prices[data.prices.size() - 2]
                : res.price;
    double p    = res.price.to_double();
    double p0   = prev.to_double();
    res.change  = (p0 != 0.0) ? ((p - p0) / p0) * 100.0 : 0.0;

    // --- SMA-50 via O(1) circular buffer ---
    CircularBuffer<Price, 50> sma_buf;
    for (auto& px : data.prices) sma_buf.push(px);
    res.sma50 = Price(sma_buf.avg());

    // --- RSI-14 (Wilder's smoothed gains/losses) ---
    const auto& px = data.prices;
    if (px.size() > 14) {
        double gain = 0.0, loss = 0.0;
        for (size_t i = px.size() - 14; i < px.size(); ++i) {
            double delta = px[i].to_double() - px[i-1].to_double();
            if (delta >= 0.0) gain += delta;
            else              loss -= delta;
        }
        if (gain == 0.0 && loss == 0.0) res.rsi = 50.0;
        else res.rsi = (loss == 0.0) ? 100.0 : 100.0 - (100.0 / (1.0 + gain / loss));
    } else {
        res.rsi = 50.0;
    }

    // --- Sentiment classification ---
    bool above_sma = res.price > res.sma50;
    if      (above_sma && res.rsi < 70.0) res.sentiment = "Bullish";
    else if (!above_sma && res.rsi > 30.0) res.sentiment = "Bearish";
    else                                   res.sentiment = "Neutral";

    res.timestamp = now_str();
    return res;
}

MarkovResult run_markov(const std::string& symbol) {
    MarkovResult res;
    res.symbol = symbol;

    auto fetched = DataEngine::fetch_prices(symbol, 2);
    if (!fetched.has_value()) {
        res.ok         = false;
        res.prediction = "Fetch Error";
        return res;
    }

    auto& data = fetched.value();
    res.source = data.source;

    if (data.prices.size() < 10) {
        res.ok         = false;
        res.prediction = "Insufficient Data";
        return res;
    }

    // --- Daily log-returns ---
    std::vector<double> rets;
    rets.reserve(data.prices.size() - 1);
    for (size_t i = 1; i < data.prices.size(); ++i) {
        double p1 = data.prices[i].to_double();
        double p0 = data.prices[i-1].to_double();
        if (p0 != 0.0) rets.push_back((p1 - p0) / p0);
    }

    // --- State thresholds: ±0.5σ ---
    double mean = std::accumulate(rets.begin(), rets.end(), 0.0) / rets.size();
    double var  = 0.0;
    for (double r : rets) var += (r - mean) * (r - mean);
    double sd = std::sqrt(var / rets.size());

    std::vector<int> states;
    states.reserve(rets.size());
    for (double r : rets)
        states.push_back(r < -0.5 * sd ? 0 : r > 0.5 * sd ? 2 : 1);

    // --- Transition count matrix ---
    double counts[3][3] = {};
    for (size_t i = 0; i + 1 < states.size(); ++i)
        counts[states[i]][states[i+1]] += 1.0;

    // --- MLE: most probable next state from current state ---
    int    last   = states.back();
    double row_total = counts[last][0] + counts[last][1] + counts[last][2];

    static const char* names[] = { "Bearish", "Neutral", "Bullish" };
    int    best   = 1;
    double best_p = 0.0;
    if (row_total > 0.0) {
        for (int i = 0; i < 3; ++i) {
            double p = counts[last][i] / row_total;
            if (p > best_p) { best_p = p; best = i; }
        }
    }
    res.prediction = names[best];
    res.confidence = best_p;

    // --- AIC validation vs random-walk null ---
    // Null log-likelihood: uniform transitions (1/3 each)
    double n_obs = static_cast<double>(states.size() - 1);
    double ll_null   = n_obs * std::log(1.0 / 3.0);   // k=0 params
    double ll_markov = 0.0;
    for (int i = 0; i < 3; ++i) {
        double rt = counts[i][0] + counts[i][1] + counts[i][2];
        if (rt > 0.0) {
            for (int j = 0; j < 3; ++j)
                if (counts[i][j] > 0.0)
                    ll_markov += counts[i][j] * std::log(counts[i][j] / rt);
        }
    }
    double aic_markov = 2.0 * 6.0 - 2.0 * ll_markov; // 6 free params (3×2)
    double aic_null   = 0.0        - 2.0 * ll_null;

    res.aic       = aic_markov;
    res.aic_valid = aic_markov < aic_null;              // Markov beats random walk

    res.timestamp = now_str();
    return res;
}

} // namespace Axiom::AnalysisEngine
