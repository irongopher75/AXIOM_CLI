#include "analysis_engine.hpp"
#include "data_engine.hpp"
#include "indicators.hpp"

#include <cmath>
#include <ctime>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include "core/buffers.hpp"

static std::string now_str() {
    auto t  = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

namespace Axiom::AnalysisEngine {

AnalysisResult run_analysis(const std::string& symbol) {
    AnalysisResult res;
    res.symbol = symbol;

    auto fetched = DataEngine::fetch_prices(symbol, 1);
    if (!fetched.has_value()) {
        res.ok = false;
        res.sentiment = "Error";
        return res;
    }

    auto& data = fetched.value();
    res.source   = data.source;
    res.exchange = data.exchange;
    res.country  = data.country;
    res.history  = data.bars;

    if (data.bars.empty()) {
        res.ok = false;
        res.sentiment = "Error";
        return res;
    }

    std::vector<double> closes;
    for (auto& b : data.bars) closes.push_back(b.c.to_double());

    res.price   = data.bars.back().c;
    Price prev  = data.bars.size() > 1 ? data.bars[data.bars.size() - 2].c : res.price;
    double p    = res.price.to_double();
    double p0   = prev.to_double();
    res.change  = (p0 != 0.0) ? ((p - p0) / p0) * 100.0 : 0.0;

    // --- Core Advanced Indicators ---
    res.sma50 = Price(Indicators::sma(closes, 50));
    res.rsi   = Indicators::rsi(closes, 14);
    res.atr14 = Price(Indicators::atr(data.bars, 14));
    auto [bb_up, bb_lo] = Indicators::bollinger_bands(closes, 20, 2.0);
    res.bb_upper = Price(bb_up);
    res.bb_lower = Price(bb_lo);
    res.vwap  = Price(Indicators::vwap(data.bars));
    res.mfi   = Indicators::mfi(data.bars, 14);
    auto adx_res = Indicators::adx(data.bars, 14);
    res.adx   = adx_res.adx;
    res.hurst = Indicators::hurst(closes);

    // --- All 50+ Stats Map ---
    auto& s = res.stats;
    s["WMA-20"]   = Indicators::wma(closes, 20);
    s["DEMA-20"]  = Indicators::dema(closes, 20);
    s["HMA-20"]   = Indicators::hma(closes, 20);
    s["KAMA-10"]  = Indicators::kama(closes, 10);
    s["STOCH_K"]  = Indicators::stochastic_k(data.bars, 14);
    s["WILL_R"]   = Indicators::williams_r(data.bars, 14);
    s["CCI-20"]   = Indicators::cci(data.bars, 20);
    auto [du, dl] = Indicators::donchian(data.bars, 20);
    s["DONCH_U"]  = du; s["DONCH_L"] = dl;
    auto [au, ad] = Indicators::aroon(data.bars, 25);
    s["AROON_U"]  = au; s["AROON_D"] = ad;
    auto lr       = Indicators::linreg(closes, 20);
    s["LR_SLOPE"] = lr.slope; s["LR_R2"] = lr.r2;
    s["ZSCORE"]   = Indicators::zscore(closes, 20);
    s["FISHER"]   = Indicators::fisher(closes, 10);
    s["STOCH_RSI"]= Indicators::stoch_rsi(closes, 14);
    s["OBV"]      = Indicators::obv(data.bars);
    s["CMF-20"]   = Indicators::cmf(data.bars, 20);
    s["PDI"]      = adx_res.pdi;
    s["NDI"]      = adx_res.ndi;

    // --- Sentiment ---
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
        res.ok = false;
        res.prediction = "Fetch Error";
        return res;
    }

    auto& data = fetched.value();
    res.source = data.source;

    if (data.bars.size() < 10) {
        res.ok = false;
        res.prediction = "Insufficient Data";
        return res;
    }

    std::vector<double> rets;
    rets.reserve(data.bars.size() - 1);
    for (size_t i = 1; i < data.bars.size(); ++i) {
        double p1 = data.bars[i].c.to_double();
        double p0 = data.bars[i-1].c.to_double();
        if (p0 != 0.0) rets.push_back((p1 - p0) / p0);
    }

    double mean = std::accumulate(rets.begin(), rets.end(), 0.0) / rets.size();
    double var  = 0.0;
    for (double r : rets) var += (r - mean) * (r - mean);
    double sd = std::sqrt(var / rets.size());

    std::vector<int> states;
    for (double r : rets) states.push_back(r < -0.5 * sd ? 0 : r > 0.5 * sd ? 2 : 1);

    double counts[3][3] = {};
    for (size_t i = 0; i + 1 < states.size(); ++i) counts[states[i]][states[i+1]] += 1.0;

    res.transition_matrix.assign(3, std::vector<double>(3, 0.0));
    for (int i = 0; i < 3; ++i) {
        double row_sum = counts[i][0] + counts[i][1] + counts[i][2];
        if (row_sum > 0.0) for (int j = 0; j < 3; ++j) res.transition_matrix[i][j] = counts[i][j] / row_sum;
    }

    int last = states.back();
    static const char* names[] = { "Bearish", "Neutral", "Bullish" };
    int best = 1; double best_p = 0.0;
    double row_total = counts[last][0] + counts[last][1] + counts[last][2];
    if (row_total > 0.0) {
        for (int i = 0; i < 3; ++i) {
            double p = counts[last][i] / row_total;
            if (p > best_p) { best_p = p; best = i; }
        }
    }
    res.prediction = names[best];
    res.confidence = best_p;

    double n_obs = static_cast<double>(states.size() - 1);
    double ll_null = n_obs * std::log(1.0 / 3.0);
    double ll_markov = 0.0;
    for (int i = 0; i < 3; ++i) {
        double rt = counts[i][0] + counts[i][1] + counts[i][2];
        if (rt > 0.0) for (int j = 0; j < 3; ++j) if (counts[i][j] > 0.0) ll_markov += counts[i][j] * std::log(counts[i][j] / rt);
    }
    double aic_markov = 2.0 * 6.0 - 2.0 * ll_markov;
    double aic_null = -2.0 * ll_null;
    res.aic = aic_markov;
    res.aic_valid = aic_markov < aic_null;

    res.timestamp = now_str();
    return res;
}

} // namespace Axiom::AnalysisEngine
