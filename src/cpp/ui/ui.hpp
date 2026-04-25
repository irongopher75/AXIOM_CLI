#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <regex>
#include "../core/types.hpp"
#include "../core/domain.hpp"

namespace Axiom::UI {

// ─── Institutional Palette ───────────────────────────────────────────────────

struct RGB { uint8_t r, g, b; };

inline std::string fg(RGB c) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "\033[38;2;%u;%u;%um", c.r, c.g, c.b);
    return buf;
}
inline std::string bg(RGB c) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "\033[48;2;%u;%u;%um", c.r, c.g, c.b);
    return buf;
}
inline const std::string RESET   = "\033[0m";
inline const std::string BOLD    = "\033[1m";
inline const std::string DIM     = "\033[2m";

inline const RGB EMERALD = {26, 158, 110};
inline const RGB MUTED   = {42, 74, 62};
inline const RGB GOLD    = {212, 175, 55};
inline const RGB CRIMSON = {184, 15, 10};

inline const std::string RED     = "\033[31m";
inline const std::string GREEN   = "\033[32m";
inline const std::string YELLOW  = "\033[33m";
inline const std::string CYAN    = "\033[36m";

// ─── Helpers ──────────────────────────────────────────────────────────────────

inline std::string repeat(const std::string& s, int n) {
    std::string out;
    out.reserve(s.size() * static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) out += s;
    return out;
}

inline std::string strip_ansi(const std::string& s) {
    return std::regex_replace(s, std::regex("\033\\[[0-9;]*[a-zA-Z]"), "");
}

inline std::string format_large(double val) {
    if (std::abs(val) >= 1e9) return (std::ostringstream() << std::fixed << std::setprecision(2) << val / 1e9 << "B").str();
    if (std::abs(val) >= 1e6) return (std::ostringstream() << std::fixed << std::setprecision(2) << val / 1e6 << "M").str();
    if (std::abs(val) >= 1e3) return (std::ostringstream() << std::fixed << std::setprecision(1) << val / 1e3 << "K").str();
    if (val == std::floor(val)) return std::to_string(static_cast<long long>(val));
    return (std::ostringstream() << std::fixed << std::setprecision(2) << val).str();
}

/**
 * @brief Renders a 3-tier sparkline.
 */
inline std::string get_chart(const std::vector<Bar>& bars) {
    if (bars.size() < 10) return "";
    int start = std::max(0, static_cast<int>(bars.size()) - 40);
    std::vector<Price> data;
    for (size_t i = start; i < bars.size(); ++i) data.push_back(bars[i].c);
    Price min_p = data[0], max_p = data[0];
    for (auto& p : data) { if (p < min_p) min_p = p; if (p > max_p) max_p = p; }
    double lo = min_p.to_double(), hi = max_p.to_double();
    double rng = (hi - lo) > 0.0 ? hi - lo : 1.0;
    std::string rows[3];
    for (auto& p : data) {
        double norm = (p.to_double() - lo) / rng;
        if      (norm > 0.66) { rows[0] += "█"; rows[1] += " "; rows[2] += " "; }
        else if (norm > 0.33) { rows[0] += " "; rows[1] += "█"; rows[2] += " "; }
        else                  { rows[0] += " "; rows[1] += " "; rows[2] += "█"; }
    }
    std::string color = fg(EMERALD);
    return color + "  HI │ " + rows[0] + "  $" + max_p.str(2) + "\n"
         + color + "  MD │ " + rows[1] + "\n"
         + color + "  LO │ " + rows[2] + "  $" + min_p.str(2) + RESET;
}

inline std::string pill(const std::string& text, RGB color) {
    return bg(color) + fg({255, 255, 255}) + " " + text + " " + RESET;
}

inline void print_panel(const std::string& content,
                         const std::string& ticker,
                         const std::string& exchange = "NYSE") {
    const int WIDTH = 84;
    
    // Header Row
    std::cout << fg(EMERALD) << "┏━ " << BOLD << ticker << RESET << fg(EMERALD) << " " << repeat("━", WIDTH - ticker.length() - 3) << "┓" << RESET << "\n";
    std::cout << fg(EMERALD) << "┃ " << RESET << DIM << std::left << std::setw(10) << exchange << RESET << "  " << pill("LIVE", EMERALD);
    int head_used = 10 + 2 + 6; // exchange + padding + pill
    std::cout << std::string(WIDTH - head_used - 2, ' ') << fg(EMERALD) << "┃" << RESET << "\n";
    std::cout << fg(EMERALD) << "┣" << repeat("━", WIDTH) << "┫" << RESET << "\n";
    
    // Content
    std::stringstream ss(content);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.empty()) {
            std::cout << fg(EMERALD) << "┃ " << RESET << std::string(WIDTH - 2, ' ') << fg(EMERALD) << "┃" << RESET << "\n";
            continue;
        }
        std::string stripped = strip_ansi(line);
        int visible_len = static_cast<int>(stripped.length());
        int padding = std::max(0, WIDTH - visible_len - 2);
        std::cout << fg(EMERALD) << "┃ " << RESET << line << std::string(padding, ' ') << fg(EMERALD) << "┃" << RESET << "\n";
    }
    
    std::cout << fg(EMERALD) << "┗" << repeat("━", WIDTH) << "┛" << RESET << "\n";
}

inline void print_splash() {
    std::cout << fg(EMERALD) << BOLD << R"(
    ___    _  _____  ____  __  ___
   /   |  | |/ /_ _/ __ \/  |/  /
  / /| |  |   / / // / / / /|_/ / 
 / ___ | /   |_/ // /_/ / /  / /  
/_/  |_|/_/|_/___/\____/_/  /_/   
                                   )" << RESET << "\n"
              << fg(MUTED) << "  Institutional Terminal v0.2.1" << RESET << "\n"
              << "  Type 'predict <ticker>' or just a ticker (e.g. 'AAPL') to start.\n\n";
}

} // namespace Axiom::UI
