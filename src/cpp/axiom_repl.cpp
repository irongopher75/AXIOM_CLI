#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <limits>

#include "engine/data_engine.hpp"
#include "engine/analysis_engine.hpp"
#include "ui/ui.hpp"
#include "core/paths.hpp"
#include "linenoise.h"

using namespace Axiom;

// ---------------------------------------------------------------------------
// Render helpers — convert engine results to ANSI strings for the panel
// ---------------------------------------------------------------------------
static std::string render_analysis(const AnalysisResult& r) {
    using namespace UI;
    std::string col = (r.sentiment == "Bullish") ? GREEN
                    : (r.sentiment == "Bearish") ? RED
                    :                              YELLOW;

    std::string chg_col = r.change >= 0.0 ? GREEN : RED;
    std::string chg_str = (r.change >= 0.0 ? "+" : "") +
                          std::to_string(r.change).substr(0, 6) + "%";

    std::string out;
    out += "  Price:     " + BOLD + "$" + r.price.str(2) + RESET +
           "  (" + chg_col + chg_str + RESET + ")\n";
    out += get_chart(r.history) + "\n";
    out += "  SMA-50:   $" + r.sma50.str(2) +
           "   RSI-14: "   + std::to_string(r.rsi).substr(0, 5) + "\n";
    out += "  Sentiment: " + col + BOLD + r.sentiment + RESET + "\n";
    out += "  Source:    " + CYAN + r.source + RESET +
           "  [" + r.timestamp + "]";
    return out;
}

static std::string render_markov(const MarkovResult& r) {
    using namespace UI;
    std::string validity = r.aic_valid ? GREEN  + "✓ model beats random walk" + RESET
                                       : YELLOW + "⚠ model ≈ random walk"    + RESET;
    std::string out;
    out += "  Prediction: " + BOLD + GREEN + r.prediction + RESET + "\n";
    out += "  Confidence: " + YELLOW + std::to_string(static_cast<int>(r.confidence * 100)) + "%" + RESET + "\n";
    out += "  AIC valid:  " + validity + "\n";
    out += "  Source:     " + CYAN + r.source + RESET +
           "  [" + r.timestamp + "]";
    return out;
}

// ---------------------------------------------------------------------------
// Symbol resolution + interactive selection menu
// ---------------------------------------------------------------------------
static std::string resolve_interactive(const std::string& query) {
    auto matches = DataEngine::resolve_symbol(query);
    if (matches.empty()) {
        std::cout << UI::RED << "  No matches found for '" << query << "'." << UI::RESET << "\n";
        return "";
    }

    // Exact ticker match — skip the menu
    if (matches.size() == 1 || matches[0].symbol == query) return matches[0].symbol;

    std::cout << UI::MAGENTA << "  Multiple matches — select one:" << UI::RESET << "\n";
    for (size_t i = 0; i < matches.size(); ++i)
        std::cout << "  " << (i + 1) << ".  " << UI::BOLD << matches[i].symbol << UI::RESET
                  << "  (" << matches[i].name << "  ·  " << matches[i].exchange << ")\n";

    std::cout << UI::CYAN << "  Select [1-" << matches.size() << "]: " << UI::RESET;
    int choice = 0;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return "";
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (choice < 1 || static_cast<size_t>(choice) > matches.size()) return "";
    return matches[static_cast<size_t>(choice) - 1].symbol;
}

// ---------------------------------------------------------------------------
// Command dispatch
// ---------------------------------------------------------------------------
static void handle_command(const std::string& input) {
    if (input.empty()) return;

    std::istringstream ss(input);
    std::string cmd;
    ss >> cmd;
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    // --- Builtins ---
    if (cmd == "help") {
        std::cout << UI::BOLD << "  Commands:" << UI::RESET << "\n"
                  << "    analyze <name|ticker>   Full technical report\n"
                  << "    predict <name|ticker>   Markov regime forecast\n"
                  << "    clear                   Reset terminal\n"
                  << "    quit / exit             Exit\n";
        return;
    }
    if (cmd == "clear") {
        std::cout << "\033[2J\033[H";
        UI::print_splash();
        return;
    }

    // --- Determine mode and extract query ---
    bool do_predict  = (cmd == "predict");
    bool do_analyze  = (cmd == "analyze" || cmd == "analyse");
    std::string query;

    if (do_predict || do_analyze) {
        std::getline(ss >> std::ws, query);
    } else {
        query      = input;   // bare ticker / name — default to analyze
        do_analyze = true;
    }

    if (query.empty()) {
        std::cout << UI::YELLOW << "  Usage: analyze <ticker>  or  predict <ticker>\n" << UI::RESET;
        return;
    }

    // --- Resolve symbol ---
    std::string sym = resolve_interactive(query);
    if (sym.empty()) return;

    // --- Dispatch to engine ---
    if (do_predict) {
        std::cout << UI::YELLOW << "  Forecasting " << sym << "..." << UI::RESET << "\n";
        auto result = AnalysisEngine::run_markov(sym);
        if (!result.ok) {
            std::cout << UI::RED << "  Error: " << result.prediction << UI::RESET << "\n";
            return;
        }
        UI::print_panel(render_markov(result), "MARKOV FORECAST: " + sym, UI::GREEN);
    } else {
        std::cout << UI::YELLOW << "  Analyzing " << sym << "..." << UI::RESET << "\n";
        auto result = AnalysisEngine::run_analysis(sym);
        if (!result.ok) {
            std::cout << UI::RED << "  Analysis failed for " << sym << UI::RESET << "\n";
            return;
        }
        std::string col = (result.sentiment == "Bullish") ? UI::GREEN
                        : (result.sentiment == "Bearish") ? UI::RED
                        :                                   UI::YELLOW;
        UI::print_panel(render_analysis(result), "ANALYSIS: " + sym, col);
    }
}

// ---------------------------------------------------------------------------
// Tab completion
// ---------------------------------------------------------------------------
static void completion(const char* buf, linenoiseCompletions* lc) {
    switch (buf[0]) {
        case 'a': linenoiseAddCompletion(lc, "analyze "); break;
        case 'p': linenoiseAddCompletion(lc, "predict "); break;
        case 'c': linenoiseAddCompletion(lc, "clear");    break;
        case 'q': linenoiseAddCompletion(lc, "quit");     break;
        case 'h': linenoiseAddCompletion(lc, "help");     break;
    }
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int main() {
    UI::print_splash();
    linenoiseSetCompletionCallback(completion);
    linenoiseHistoryLoad(Axiom::Paths::get_path("history").c_str());

    char* raw;
    while (true) {
        auto t  = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::ostringstream prompt;
        prompt << UI::CYAN << "[" << std::put_time(&tm, "%H:%M:%S") << "] "
               << UI::RESET << "axiom > ";

        raw = linenoise(prompt.str().c_str());
        if (!raw) break;

        std::string line(raw);
        free(raw);

        if (line == "quit" || line == "exit") break;
        if (!line.empty()) {
            std::cout << "\n";
            handle_command(line);
            std::cout << "\n";
            linenoiseHistoryAdd(line.c_str());
            linenoiseHistorySave(Axiom::Paths::get_path("history").c_str());
        }
    }
    return 0;
}
