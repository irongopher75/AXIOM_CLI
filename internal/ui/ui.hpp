#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <regex>
#include "core/types.hpp"
#include "core/domain.hpp"

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

inline int column_width(const std::string& s) {
    std::string stripped = strip_ansi(s);
    int width = 0;
    for (size_t i = 0; i < stripped.length(); ) {
        unsigned char c = static_cast<unsigned char>(stripped[i]);
        if (c <= 127) { width += 1; i += 1; }
        else if ((c & 0xE0) == 0xC0) { width += 1; i += 2; }
        else if ((c & 0xF0) == 0xE0) { width += 1; i += 3; }
        else if ((c & 0xF8) == 0xF0) { width += 1; i += 4; }
        else { i += 1; } // Fallback
    }
    return width;
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
    const int W = 80; // Internal Column Width
    const int TOTAL = W + 2;
    
    // Top: ┏━ TSLA ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
    std::string head_prefix = "┏━ " + ticker + " ";
    int head_w = column_width(head_prefix);
    std::cout << fg(EMERALD) << head_prefix << repeat("━", TOTAL - head_w - 1) << "┓" << RESET << "\n";
    
    // Sub-header: ┃ NYSE  [LIVE]                                                   ┃
    std::string sub = "┃ " + DIM + exchange + RESET + "  " + pill("LIVE", EMERALD);
    int sub_w = column_width(sub);
    std::cout << fg(EMERALD) << sub << RESET << std::string(TOTAL - sub_w - 1, ' ') << fg(EMERALD) << "┃" << RESET << "\n";
    
    // Separator: ┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
    std::cout << fg(EMERALD) << "┣" << repeat("━", W) << "┫" << RESET << "\n";
    
    // Content
    std::stringstream ss(content);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.empty()) {
            std::cout << fg(EMERALD) << "┃" << std::string(W, ' ') << "┃" << RESET << "\n";
            continue;
        }
        int line_w = column_width(line);
        std::cout << fg(EMERALD) << "┃ " << RESET << line << std::string(std::max(0, TOTAL - line_w - 3), ' ') << fg(EMERALD) << " ┃" << RESET << "\n";
    }
    
    // Bottom: ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
    std::cout << fg(EMERALD) << "┗" << repeat("━", W) << "┛" << RESET << "\n";
}

inline void print_splash() {
    std::cout << fg(EMERALD) << BOLD << R"(
    ___    _  _____  ____  __  ___
   /   |  | |/ /_ _/ __ \/  |/  /
  / /| |  |   / / // / / / /|_/ / 
 / ___ | /   |_/ // /_/ / /  / /  
/_/  |_|/_/|_/___/\____/_/  /_/   
                                   )" << RESET << "\n"
              << fg(MUTED) << "  Institutional Terminal v0.4.0-alpha" << RESET << "\n"
              << "  Type 'predict <ticker>' or just a ticker (e.g. 'AAPL') to start.\n\n";
}

} // namespace Axiom::UI
