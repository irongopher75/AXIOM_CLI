#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>

#include "core/domain.hpp"
#include "engine/data_engine.hpp"
#include "engine/analysis_engine.hpp"
#include "ui/ui.hpp"
#include "ui/heatmap.hpp"
#include "core/paths.hpp"
#include "linenoise.h"

using namespace Axiom;

// ---------------------------------------------------------------------------
// Globals & State
// ---------------------------------------------------------------------------
enum class ReplState { Ticker, Selection };
enum class RunMode   { Both, Analyze, Predict };

static ReplState current_state = ReplState::Ticker;
static RunMode   current_mode  = RunMode::Both;
static std::vector<SymbolMatch> pending_matches;

// ---------------------------------------------------------------------------
// UI Rendering Helpers
// ---------------------------------------------------------------------------

static std::string render_analysis(const AnalysisResult& r) {
    using namespace UI;
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
    
    out << "  " << DIM << "Source: " << r.source << "  ·  " << r.timestamp << RESET;

    return out.str();
}

static std::string render_markov(const MarkovResult& r) {
    using namespace UI;
    if (!r.ok) return fg(CRIMSON) + "  Forecasting failed." + RESET;

    std::stringstream out;
    out << "  Prediction: " << BOLD << fg(EMERALD) << r.prediction << RESET << "\n";
    out << "  Confidence: " << fg(GOLD) << static_cast<int>(r.confidence * 100) << "%" << RESET;
    
    // Clarify validity threshold
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

// ---------------------------------------------------------------------------
// Logic Handlers
// ---------------------------------------------------------------------------

static void execute_analysis(const SymbolMatch& m) {
    if (current_mode == RunMode::Both || current_mode == RunMode::Analyze) {
        std::cout << UI::fg(UI::MUTED) << "  Analyzing " << m.symbol << "..." << UI::RESET << "\r";
        std::cout.flush();
        auto res = AnalysisEngine::run_analysis(m.symbol);
        UI::print_panel(render_analysis(res), m.symbol, m.exchange);
    }

    if (current_mode == RunMode::Both || current_mode == RunMode::Predict) {
        std::cout << UI::fg(UI::MUTED) << "  Forecasting " << m.symbol << "..." << UI::RESET << "\r";
        std::cout.flush();
        auto mres = AnalysisEngine::run_markov(m.symbol);
        UI::print_panel(render_markov(mres), "REGIME: " + m.symbol, "MARKOV MLE");
    }
    std::cout << "\n";
}

static void handle_input(const std::string& input) {
    if (input.empty()) return;

    std::string cmd, target;
    std::string lower_input = input;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);

    if (lower_input == "help") {
        std::cout << "\n  " << UI::BOLD << UI::fg(UI::EMERALD) << "COMMAND REFERENCE" << UI::RESET << "\n"
                  << "  " << UI::fg(UI::MUTED) << "-----------------" << UI::RESET << "\n"
                  << "  " << std::left << std::setw(18) << "predict <ticker>" << "Run Markov regime forecasting (e.g. 'p AAPL')\n"
                  << "  " << std::left << std::setw(18) << "analyze <ticker>" << "Run full technical indicator suite (e.g. 'a TSLA')\n"
                  << "  " << std::left << std::setw(18) << "<ticker>" << "Run combined report (default)\n"
                  << "  " << std::left << std::setw(18) << "clear" << "Clear the terminal screen\n"
                  << "  " << std::left << std::setw(18) << "exit / quit" << "Exit the Axiom terminal\n\n";
        return;
    }
    if (lower_input == "clear") {
        std::cout << "\033[2J\033[H";
        return;
    }

    if (lower_input.starts_with("p ") || lower_input.starts_with("predict ")) {
        current_mode = RunMode::Predict;
        size_t space = input.find(' ');
        target = input.substr(space + 1);
    } else if (lower_input.starts_with("a ") || lower_input.starts_with("analyze ")) {
        current_mode = RunMode::Analyze;
        size_t space = input.find(' ');
        target = input.substr(space + 1);
    } else {
        current_mode = RunMode::Both;
        target = input;
    }

    // Trim target
    target.erase(0, target.find_first_not_of(" \t"));
    target.erase(target.find_last_not_of(" \t") + 1);

    auto matches = DataEngine::resolve_symbol(target);
    if (matches.empty()) {
        std::cout << UI::fg(UI::CRIMSON) << "  No matches found for '" << target << "'" << UI::RESET << "\n\n";
        return;
    }

    std::string up = target;
    std::transform(up.begin(), up.end(), up.begin(), ::toupper);
    for (const auto& m : matches) {
        if (m.symbol == up) {
            execute_analysis(m);
            return;
        }
    }

    if (matches.size() == 1) {
        execute_analysis(matches[0]);
    } else {
        std::cout << "\n  " << UI::fg(UI::MUTED) << "Multiple matches — select one:" << UI::RESET << "\n";
        for (size_t i = 0; i < matches.size(); ++i) {
            std::cout << "  " << (i + 1) << ".  " << UI::BOLD << UI::fg(UI::EMERALD) << matches[i].symbol << UI::RESET 
                      << "  " << UI::DIM << matches[i].name << "  ·  " << matches[i].exchange << UI::RESET << "\n";
        }
        pending_matches = matches;
        current_state = ReplState::Selection;
    }
}

static void handle_selection_input(const std::string& input) {
    if (input == "q" || input == "0" || input == "quit" || input == "exit" || input == "\x1b") {
        std::cout << "  Cancelled.\n\n";
        current_state = ReplState::Ticker;
        return;
    }

    if (!input.empty() && std::all_of(input.begin(), input.end(), ::isdigit)) {
        int idx = std::stoi(input) - 1;
        if (idx >= 0 && idx < static_cast<int>(pending_matches.size())) {
            execute_analysis(pending_matches[idx]);
            current_state = ReplState::Ticker;
            return;
        }
    }

    std::string up = input;
    std::transform(up.begin(), up.end(), up.begin(), ::toupper);
    for (const auto& m : pending_matches) {
        if (m.symbol == up) {
            execute_analysis(m);
            current_state = ReplState::Ticker;
            return;
        }
    }

    std::cout << UI::fg(UI::GOLD) << "  Selection required. " << UI::RESET 
              << "Enter 1-" << pending_matches.size() << " or 'q': ";
    std::cout.flush();
}

// ---------------------------------------------------------------------------
// Main Loop
// ---------------------------------------------------------------------------

int main() {
    Axiom::Paths::ensure_axiom_dir();
    UI::print_splash();

    char* line;
    std::string h_path = Axiom::Paths::get_path("history");
    linenoiseHistoryLoad(h_path.c_str());

    while (true) {
        std::string prompt = (current_state == ReplState::Ticker) 
                           ? "\033[38;2;26;158;110maxiom > \033[0m" 
                           : "  Select [1-" + std::to_string(pending_matches.size()) + ", q=cancel]: ";
        
        line = linenoise(prompt.c_str());
        if (!line) break;

        std::string input(line);
        free(line);
        
        input.erase(0, input.find_first_not_of(" \t\n\r"));
        input.erase(input.find_last_not_of(" \t\n\r") + 1);

        if (input == "exit" || input == "quit") break;
        if (input.empty()) continue;

        linenoiseHistoryAdd(input.c_str());
        linenoiseHistorySave(h_path.c_str());

        if (current_state == ReplState::Ticker) {
            handle_input(input);
        } else {
            handle_selection_input(input);
        }
    }

    return 0;
}
