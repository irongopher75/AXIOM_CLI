#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include "../core/types.hpp"

// ---------------------------------------------------------------------------
// All terminal rendering lives here.  Nothing in this header knows about
// curl, JSON, Markov chains, or rate limiters.
// ---------------------------------------------------------------------------

namespace Axiom::UI {

// ANSI codes
inline const std::string RESET   = "\033[0m";
inline const std::string BOLD    = "\033[1m";
inline const std::string RED     = "\033[31m";
inline const std::string GREEN   = "\033[32m";
inline const std::string YELLOW  = "\033[33m";
inline const std::string BLUE    = "\033[34m";
inline const std::string MAGENTA = "\033[35m";
inline const std::string CYAN    = "\033[36m";

// ---------------------------------------------------------------------------
// Repeat a string n times
// ---------------------------------------------------------------------------
inline std::string repeat(const std::string& s, int n) {
    std::string out;
    out.reserve(s.size() * static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) out += s;
    return out;
}

// ---------------------------------------------------------------------------
// Render a 3-tier bar chart from raw price history.
// The analysis engine stores prices; the REPL calls this at display time.
// ---------------------------------------------------------------------------
inline std::string get_chart(const std::vector<Price>& prices) {
    if (prices.size() < 10) return "";

    int start = std::max(0, static_cast<int>(prices.size()) - 30);
    std::vector<Price> data(prices.begin() + start, prices.end());

    Price min_p = *std::min_element(data.begin(), data.end());
    Price max_p = *std::max_element(data.begin(), data.end());
    double lo   = min_p.to_double();
    double hi   = max_p.to_double();
    double rng  = (hi - lo) > 0.0 ? hi - lo : 1.0;

    std::string rows[3];
    for (auto& p : data) {
        double norm = (p.to_double() - lo) / rng;
        if      (norm > 0.66) { rows[0] += "█"; rows[1] += " "; rows[2] += " "; }
        else if (norm > 0.33) { rows[0] += " "; rows[1] += "█"; rows[2] += " "; }
        else                  { rows[0] += " "; rows[1] += " "; rows[2] += "█"; }
    }

    return GREEN  + "  HI │ " + rows[0] + " $" + max_p.str(2) + "\n"
         + YELLOW + "  MD │ " + rows[1] + "\n"
         + RED    + "  LO │ " + rows[2] + " $" + min_p.str(2) + RESET;
}

// ---------------------------------------------------------------------------
// Box-drawing panel with a coloured title bar
// ---------------------------------------------------------------------------
inline void print_panel(const std::string& content,
                         const std::string& title = "AXIOM",
                         const std::string& color = CYAN) {
    const int WIDTH = 48;
    int pad = WIDTH - static_cast<int>(title.length()) - 3;
    std::cout << color << "┏━ " << BOLD << title << " "
              << repeat("━", std::max(0, pad)) << RESET << "\n"
              << content << "\n"
              << color << "┗" << repeat("━", WIDTH) << RESET << "\n";
}

// ---------------------------------------------------------------------------
// Splash screen
// ---------------------------------------------------------------------------
inline void print_splash() {
    std::cout << CYAN << BOLD << R"(
    ___    _  _____  ____  __  ___
   /   |  | |/ /_ _/ __ \/  |/  /
  / /| |  |   / / // / / / /|_/ / 
 / ___ | /   |_/ // /_/ / /  / /  
/_/  |_|/_/|_/___/\____/_/  /_/   
                                   )" << RESET << "\n"
              << MAGENTA << "  Institutional Terminal v0.1.1" << RESET << "\n"
              << "  Type a name or ticker (e.g. 'Tesla' or 'AAPL') to start.\n\n";
}

} // namespace Axiom::UI
