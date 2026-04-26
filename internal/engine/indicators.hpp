#pragma once
#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <deque>
#include "core/domain.hpp"

namespace Axiom::Indicators {

// ─── UTILITIES ───────────────────────────────────────────────────────────────

inline double sum(const std::vector<double>& data, size_t period) {
    if (data.size() < period) return 0.0;
    return std::accumulate(data.end() - period, data.end(), 0.0);
}

inline double stddev(const std::vector<double>& data, size_t period) {
    if (data.size() < period) return 0.0;
    double avg = sum(data, period) / period;
    double sq_sum = 0;
    for (size_t i = data.size() - period; i < data.size(); ++i)
        sq_sum += (data[i] - avg) * (data[i] - avg);
    return std::sqrt(sq_sum / period);
}

// ─── TREND / MOVING AVERAGES ─────────────────────────────────────────────────

inline double sma(const std::vector<double>& data, size_t period) {
    if (data.size() < period) return 0.0;
    return sum(data, period) / period;
}

inline double ema(const std::vector<double>& data, size_t period) {
    if (data.size() < period) return 0.0;
    double alpha = 2.0 / (period + 1.0);
    double val = data[0]; 
    for (size_t i = 1; i < data.size(); ++i) val = alpha * data[i] + (1.0 - alpha) * val;
    return val;
}

inline double wma(const std::vector<double>& data, size_t period) {
    if (data.size() < period) return 0.0;
    double weight_sum = (period * (period + 1)) / 2.0;
    double val = 0;
    for (size_t i = 0; i < period; ++i)
        val += data[data.size() - period + i] * (i + 1);
    return val / weight_sum;
}

inline double dema(const std::vector<double>& data, size_t period) {
    if (data.size() < period * 2) return 0.0;
    std::vector<double> e1;
    double alpha = 2.0 / (period + 1.0);
    double v1 = data[0];
    for (double d : data) { v1 = alpha * d + (1.0 - alpha) * v1; e1.push_back(v1); }
    double v2 = e1[0];
    std::vector<double> e2;
    for (double d : e1) { v2 = alpha * d + (1.0 - alpha) * v2; e2.push_back(v2); }
    return 2.0 * e1.back() - e2.back();
}

inline double hma(const std::vector<double>& data, size_t period) {
    if (data.size() < period) return 0.0;
    std::vector<double> half_wma, full_wma;
    for (size_t i = period / 2; i <= data.size(); ++i) {
        std::vector<double> sub(data.begin(), data.begin() + i);
        half_wma.push_back(wma(sub, period / 2));
    }
    for (size_t i = period; i <= data.size(); ++i) {
        std::vector<double> sub(data.begin(), data.begin() + i);
        full_wma.push_back(wma(sub, period));
    }
    std::vector<double> diff;
    size_t n = std::min(half_wma.size(), full_wma.size());
    for (size_t i = 0; i < n; ++i)
        diff.push_back(2.0 * half_wma[half_wma.size() - n + i] - full_wma[full_wma.size() - n + i]);
    return wma(diff, static_cast<size_t>(std::sqrt(period)));
}

inline double kama(const std::vector<double>& data, size_t period = 10, size_t fast = 2, size_t slow = 30) {
    if (data.size() < period + 1) return 0.0;
    double er = std::abs(data.back() - data[data.size() - period]) / 
                std::accumulate(data.end() - period, data.end(), 0.0, [](double s, double v){ return s + std::abs(v); });
    double sc = std::pow(er * (2.0/(fast+1) - 2.0/(slow+1)) + 2.0/(slow+1), 2);
    static double last_kama = data[0];
    last_kama = last_kama + sc * (data.back() - last_kama);
    return last_kama;
}

// ─── MOMENTUM / OSCILLATORS ──────────────────────────────────────────────────

inline double rsi(const std::vector<double>& data, size_t period = 14) {
    if (data.size() < period + 1) return 50.0;
    double g = 0, l = 0;
    for (size_t i = data.size() - period; i < data.size(); ++i) {
        double d = data[i] - data[i-1];
        if (d > 0) g += d; else l -= d;
    }
    if (g == 0 && l == 0) return 50.0;
    return (l == 0) ? 100.0 : 100.0 - (100.0 / (1.0 + g / l));
}

struct MACD { double macd, signal, hist; };
inline MACD macd(const std::vector<double>& data, size_t fast = 12, size_t slow = 26, size_t sig = 9) {
    if (data.size() < slow + sig) return {0,0,0};
    std::vector<double> macd_line;
    for (size_t i = slow; i <= data.size(); ++i) {
        std::vector<double> sub(data.begin(), data.begin() + i);
        macd_line.push_back(ema(sub, fast) - ema(sub, slow));
    }
    double signal = ema(macd_line, sig);
    return {macd_line.back(), signal, macd_line.back() - signal};
}

inline double stochastic_k(const std::vector<Bar>& bars, size_t period = 14) {
    if (bars.size() < period) return 50.0;
    double low = bars.back().l.to_double(), high = bars.back().h.to_double();
    for (size_t i = bars.size() - period; i < bars.size(); ++i) {
        low = std::min(low, bars[i].l.to_double());
        high = std::max(high, bars[i].h.to_double());
    }
    if (high == low) return 50.0;
    return 100.0 * (bars.back().c.to_double() - low) / (high - low);
}

inline double williams_r(const std::vector<Bar>& bars, size_t period = 14) {
    if (bars.size() < period) return -50.0;
    double low = bars.back().l.to_double(), high = bars.back().h.to_double();
    for (size_t i = bars.size() - period; i < bars.size(); ++i) {
        low = std::min(low, bars[i].l.to_double());
        high = std::max(high, bars[i].h.to_double());
    }
    if (high == low) return -50.0;
    return -100.0 * (high - bars.back().c.to_double()) / (high - low);
}

inline double cci(const std::vector<Bar>& bars, size_t period = 20) {
    if (bars.size() < period) return 0.0;
    std::vector<double> tp;
    for (auto& b : bars) tp.push_back((b.h.to_double() + b.l.to_double() + b.c.to_double()) / 3.0);
    double ma = sma(tp, period);
    double md = 0;
    for (size_t i = tp.size() - period; i < tp.size(); ++i) md += std::abs(tp[i] - ma);
    md /= period;
    if (md == 0) return 0.0;
    return (tp.back() - ma) / (0.015 * md);
}

// ─── VOLATILITY ──────────────────────────────────────────────────────────────

inline double atr(const std::vector<Bar>& bars, size_t period = 14) {
    if (bars.size() < period + 1) return 0.0;
    std::vector<double> trs;
    for (size_t i = 1; i < bars.size(); ++i) {
        double hl = bars[i].h.to_double() - bars[i].l.to_double();
        double hpc = std::abs(bars[i].h.to_double() - bars[i-1].c.to_double());
        double lpc = std::abs(bars[i].l.to_double() - bars[i-1].c.to_double());
        trs.push_back(std::max({hl, hpc, lpc}));
    }
    return ema(trs, period);
}

inline std::pair<double, double> bollinger_bands(const std::vector<double>& data, size_t period = 20, double sd = 2.0) {
    double ma = sma(data, period);
    double dev = stddev(data, period);
    return {ma + sd * dev, ma - sd * dev};
}

inline std::pair<double, double> keltner_channels(const std::vector<Bar>& bars, size_t period = 20, double mult = 2.0) {
    std::vector<double> closes;
    for (auto& b : bars) closes.push_back(b.c.to_double());
    double ma = ema(closes, period);
    double a = atr(bars, period);
    return {ma + mult * a, ma - mult * a};
}

inline std::pair<double, double> donchian(const std::vector<Bar>& bars, size_t period = 20) {
    if (bars.size() < period) return {0,0};
    double h = bars.back().h.to_double(), l = bars.back().l.to_double();
    for (size_t i = bars.size() - period; i < bars.size(); ++i) {
        h = std::max(h, bars[i].h.to_double());
        l = std::min(l, bars[i].l.to_double());
    }
    return {h, l};
}

// ─── VOLUME ──────────────────────────────────────────────────────────────────

inline double obv(const std::vector<Bar>& bars) {
    double obv_val = 0;
    for (size_t i = 1; i < bars.size(); ++i) {
        if (bars[i].c > bars[i-1].c) obv_val += bars[i].v;
        else if (bars[i].c < bars[i-1].c) obv_val -= bars[i].v;
    }
    return obv_val;
}

inline double vwap(const std::vector<Bar>& bars) {
    double pv = 0, v = 0;
    for (auto& b : bars) {
        double tp = (b.h.to_double() + b.l.to_double() + b.c.to_double()) / 3.0;
        pv += tp * b.v; v += b.v;
    }
    return v > 0 ? pv / v : 0.0;
}

inline double cmf(const std::vector<Bar>& bars, size_t period = 20) {
    if (bars.size() < period) return 0.0;
    double mfv_sum = 0, v_sum = 0;
    for (size_t i = bars.size() - period; i < bars.size(); ++i) {
        double h = bars[i].h.to_double(), l = bars[i].l.to_double(), c = bars[i].c.to_double();
        double mfm = (h == l) ? 0 : ((c - l) - (h - c)) / (h - l);
        mfv_sum += mfm * bars[i].v;
        v_sum += bars[i].v;
    }
    return v_sum > 0 ? mfv_sum / v_sum : 0.0;
}

inline double mfi(const std::vector<Bar>& bars, size_t period = 14) {
    if (bars.size() < period + 1) return 50.0;
    double pos = 0, neg = 0;
    for (size_t i = bars.size() - period; i < bars.size(); ++i) {
        double tp = (bars[i].h.to_double() + bars[i].l.to_double() + bars[i].c.to_double()) / 3.0;
        double ptp = (bars[i-1].h.to_double() + bars[i-1].l.to_double() + bars[i-1].c.to_double()) / 3.0;
        if (tp > ptp) pos += tp * bars[i].v;
        else if (tp < ptp) neg += tp * bars[i].v;
    }
    return neg == 0 ? 100.0 : 100.0 - (100.0 / (1.0 + pos / neg));
}

// ─── TREND STRENGTH / DIRECTION ───────────────────────────────────────────────

struct ADX { double adx, pdi, ndi; };
inline ADX adx(const std::vector<Bar>& bars, size_t period = 14) {
    if (bars.size() < 2 * period) return {0,0,0};
    std::vector<double> trs, pdm, ndm;
    for (size_t i = 1; i < bars.size(); ++i) {
        double h = bars[i].h.to_double(), l = bars[i].l.to_double();
        double ph = bars[i-1].h.to_double(), pl = bars[i-1].l.to_double(), pc = bars[i-1].c.to_double();
        trs.push_back(std::max({h - l, std::abs(h - pc), std::abs(l - pc)}));
        double dp = h - ph, dn = pl - l;
        pdm.push_back((dp > dn && dp > 0) ? dp : 0);
        ndm.push_back((dn > dp && dn > 0) ? dn : 0);
    }
    double str = ema(trs, period), sp = ema(pdm, period), sn = ema(ndm, period);
    double pdi = 100.0 * (str > 0 ? sp / str : 0), ndi = 100.0 * (str > 0 ? sn / str : 0);
    double dx = (pdi + ndi > 0) ? 100.0 * std::abs(pdi - ndi) / (pdi + ndi) : 0;
    return {dx, pdi, ndi};
}

inline std::pair<double, double> aroon(const std::vector<Bar>& bars, size_t period = 25) {
    if (bars.size() < period) return {0,0};
    int hh = 0, ll = 0;
    double hmax = -1e308, lmin = 1e308;
    for (int i = 0; i < (int)period; ++i) {
        size_t idx = bars.size() - period + i;
        if (bars[idx].h.to_double() >= hmax) { hmax = bars[idx].h.to_double(); hh = i; }
        if (bars[idx].l.to_double() <= lmin) { lmin = bars[idx].l.to_double(); ll = i; }
    }
    return {100.0 * hh / (period - 1), 100.0 * ll / (period - 1)};
}

inline double psar(const std::vector<Bar>& bars, [[maybe_unused]] double accel = 0.02, [[maybe_unused]] double max_accel = 0.2) {
    if (bars.size() < 2) return bars.back().c.to_double();
    // Simplified logic: for full accuracy PSAR needs state persistence.
    // This is an approximation of the last step.
    return bars.back().l.to_double(); // Placeholder
}

// ─── STATISTICAL ─────────────────────────────────────────────────────────────

struct LinReg { double slope, intercept, r2; };
inline LinReg linreg(const std::vector<double>& data, size_t period) {
    if (data.size() < period) return {0,0,0};
    double x_sum = 0, y_sum = 0, xy_sum = 0, x2_sum = 0, y2_sum = 0;
    for (size_t i = 0; i < period; ++i) {
        double x = i, y = data[data.size() - period + i];
        x_sum += x; y_sum += y; xy_sum += x * y; x2_sum += x * x; y2_sum += y * y;
    }
    double n = period;
    double slope = (n * xy_sum - x_sum * y_sum) / (n * x2_sum - x_sum * x_sum);
    double intercept = (y_sum - slope * x_sum) / n;
    double num = (n * xy_sum - x_sum * y_sum);
    double den = std::sqrt((n * x2_sum - x_sum * x_sum) * (n * y2_sum - y_sum * y_sum));
    double r2 = (den == 0) ? 0 : std::pow(num / den, 2);
    return {slope, intercept, r2};
}

inline double zscore(const std::vector<double>& data, size_t period) {
    if (data.size() < period) return 0;
    double ma = sma(data, period);
    double dev = stddev(data, period);
    return (dev == 0) ? 0 : (data.back() - ma) / dev;
}

inline double hurst(const std::vector<double>& data) {
    if (data.size() < 20) return 0.5;
    std::vector<double> r;
    for (size_t i = 1; i < data.size(); ++i) r.push_back(std::log(data[i]/data[i-1]));
    double avg = std::accumulate(r.begin(), r.end(), 0.0) / r.size();
    double cum = 0; std::vector<double> dev;
    for (double v : r) { cum += (v - avg); dev.push_back(cum); }
    double range = *std::max_element(dev.begin(), dev.end()) - *std::min_element(dev.begin(), dev.end());
    double var = 0;
    for (double v : r) var += (v - avg) * (v - avg);
    double s = std::sqrt(var / r.size());
    return (s == 0) ? 0.5 : std::log(range / s) / std::log(r.size());
}

// ─── CYCLE / REGIME ──────────────────────────────────────────────────────────

inline double fisher(const std::vector<double>& data, size_t period = 10) {
    if (data.size() < period) return 0;
    double mn = data.back(), mx = data.back();
    for (size_t i = data.size() - period; i < data.size(); ++i) {
        mn = std::min(mn, data[i]); mx = std::max(mx, data[i]);
    }
    double x = (mx == mn) ? 0 : 2.0 * ((data.back() - mn) / (mx - mn) - 0.5);
    x = std::clamp(x, -0.999, 0.999);
    static double val = 0;
    val = 0.33 * 2.0 * x + 0.67 * val;
    return 0.5 * std::log((1.0 + val) / (1.0 - val));
}

inline double stoch_rsi(const std::vector<double>& data, size_t period = 14) {
    if (data.size() < period * 2) return 50.0;
    std::vector<double> rsi_vals;
    for (size_t i = period; i <= data.size(); ++i) {
        std::vector<double> sub(data.begin(), data.begin() + i);
        rsi_vals.push_back(rsi(sub, period));
    }
    double lo = rsi_vals.back(), hi = rsi_vals.back();
    for (size_t i = rsi_vals.size() - period; i < rsi_vals.size(); ++i) {
        lo = std::min(lo, rsi_vals[i]); hi = std::max(hi, rsi_vals[i]);
    }
    if (hi == lo) return 50.0;
    return 100.0 * (rsi_vals.back() - lo) / (hi - lo);
}

} // namespace Axiom::Indicators
