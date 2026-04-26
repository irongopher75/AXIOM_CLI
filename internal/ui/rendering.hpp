#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include "ui.hpp"
#include "heatmap.hpp"
#include "core/domain.hpp"

namespace Axiom::UI {

inline std::string render_analysis(const AnalysisResult& r) {
    if (!r.ok) {
        return fg(CRIMSON) + "  Analysis failed for " + r.symbol + RESET + "\n" +
               "  Reason: Data source unreachable or ticker mismatch.";
    }

    std::stringstream out;
    double abs_change = r.price.to_double() * (r.change / 100.0);
    std::string color_str = (r.change >= 0 ? fg(EMERALD) : fg(CRIMSON));
    std::string sign = (r.change >= 0 ? "+" : "");
    
    out << "  " << BOLD << "$" << r.price.str(2) << RESET << "  "
        << color_str << sign << std::fixed << std::setprecision(2) << abs_change
        << " (" << sign << std::fixed << std::setprecision(2) << r.change << "%)" << RESET << "\n\n";

    out << "  " << pill(r.sentiment == "Bullish" ? "BULLISH REGIME" : (r.sentiment == "Bearish" ? "BEARISH REGIME" : "NEUTRAL REGIME"), 
                       r.sentiment == "Bullish" ? EMERALD : (r.sentiment == "Bearish" ? CRIMSON : MUTED));
    
    std::string rsi_state = (r.rsi > 70 ? "OVERBOUGHT" : (r.rsi < 30 ? "OVERSOLD" : "RSI NEUTRAL"));
    out << " " << pill(rsi_state, r.rsi > 70 ? CRIMSON : (r.rsi < 30 ? EMERALD : MUTED));
    
    bool above_sma = r.price > r.sma50;
    out << " " << pill(above_sma ? "ABOVE SMA-50" : "BELOW SMA-50", above_sma ? EMERALD : CRIMSON);

    std::string adx_str = (r.adx > 40 ? "STRONG TREND" : (r.adx > 20 ? "TRENDING" : "RANGING"));
    out << " " << pill(adx_str, r.adx > 20 ? EMERALD : MUTED);

    std::string hurst_str = (r.hurst > 0.55 ? "PERSISTENT" : (r.hurst < 0.45 ? "ANTI-PERSISTENT" : "RANDOM"));
    out << " " << pill(hurst_str, std::abs(r.hurst - 0.5) > 0.05 ? EMERALD : MUTED) << "\n\n";

    out << "  " << fg(MUTED) << std::left << std::setw(12) << "SMA-50" << RESET << "$" << r.sma50.str(2)
        << std::string(10, ' ') << fg(MUTED) << std::left << std::setw(12) << "RSI-14" << RESET << std::fixed << std::setprecision(1) << r.rsi << "\n";
    out << "  " << fg(MUTED) << std::left << std::setw(12) << "VWAP" << RESET << "$" << r.vwap.str(2)
        << std::string(10, ' ') << fg(MUTED) << std::left << std::setw(12) << "MFI-14" << RESET << std::fixed << std::setprecision(1) << r.mfi << "\n";
    out << "  " << fg(MUTED) << std::left << std::setw(12) << "ATR-14" << RESET << "$" << r.atr14.str(2)
        << std::string(10, ' ') << fg(MUTED) << std::left << std::setw(12) << "HURST" << RESET << std::fixed << std::setprecision(2) << r.hurst << "\n";
    out << "  " << fg(MUTED) << std::left << std::setw(12) << "BB RANGE" << RESET << r.bb_lower.str(2) << "-" << r.bb_upper.str(2) << "\n\n";

    // Technical Profile - Categorized
    std::map<std::string, std::vector<std::string>> categories = {
        {"TREND", {"WMA-20", "DEMA-20", "HMA-20", "KAMA-10", "LR_SLOPE"}},
        {"MOMENTUM", {"STOCH_K", "WILL_R", "CCI-20", "STOCH_RSI", "FISHER"}},
        {"VOL/VOL", {"DONCH_U", "DONCH_L", "ZSCORE", "OBV", "CMF-20"}}
    };

    for (auto const& [cat, keys] : categories) {
        out << "  " << BOLD << fg(MUTED) << cat << RESET << "\n";
        int count = 0;
        for (const auto& k : keys) {
            if (r.stats.count(k)) {
                out << "  " << fg(MUTED) << std::left << std::setw(12) << k << RESET 
                    << std::right << std::setw(8) << format_large(r.stats.at(k));
                if (++count % 3 == 0) out << "\n";
                else out << std::string(4, ' ');
            }
        }
        if (count % 3 != 0) out << "\n";
        out << "\n";
    }

    std::string chart = get_chart(r.history);
    if (!chart.empty()) out << chart << "\n";
    
    out << "  " << DIM << "Source: " << r.source << "  ·  " << r.timestamp << RESET << "\n";
    if (!r.exchange.empty()) {
        out << "  " << DIM << "Market: " << r.exchange << " [" << r.country << "]" << RESET;
    }

    return out.str();
}

inline std::string render_markov(const MarkovResult& r) {
    if (!r.ok) return fg(CRIMSON) + "  Forecasting failed." + RESET;

    std::stringstream out;
    out << "  Prediction: " << BOLD << fg(EMERALD) << r.prediction << RESET << "\n";
    out << "  Confidence: " << fg(GOLD) << static_cast<int>(r.confidence * 100) << "%" << RESET;
    
    if (r.aic_valid) {
        out << "  " << pill("VALID", EMERALD) << " " << DIM << "(AIC < Null)" << RESET << "\n";
    } else {
        out << "  " << pill("WEAK", MUTED) << " " << DIM << "(High Entropy)" << RESET << "\n";
    }
    
    if (!r.transition_matrix.empty()) {
        out << "\n";
        HeatmapOptions hopts;
        hopts.cell_width = 12;
        hopts.cell_height = 1;
        hopts.show_colorbar = true;
        hopts.palette = Palette::Viridis;
        
        auto hm = Heatmap::markov(r.transition_matrix, {"Bearish", "Neutral", "Bullish"}, hopts);
        out << hm.to_string();
    }

    out << "  " << DIM << "Source: " << r.source << RESET;

    return out.str();
}

} // namespace Axiom::UI
