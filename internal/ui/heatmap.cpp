#include "heatmap.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace Axiom::UI {

// Color mapping: low (red) -> medium (yellow) -> high (green)
RGB Heatmap::prob_to_color(double p) {
    p = std::clamp(p, 0.0, 1.0);
    if (p < 0.33) {
        // Red to Yellow
        double t = p / 0.33;
        return {
            static_cast<uint8_t>(255),
            static_cast<uint8_t>(255 * t),
            static_cast<uint8_t>(0)
        };
    } else if (p < 0.66) {
        // Yellow to Green
        double t = (p - 0.33) / 0.33;
        return {
            static_cast<uint8_t>(255 * (1 - t)),
            static_cast<uint8_t>(255),
            static_cast<uint8_t>(0)
        };
    } else {
        // Green to Bright Green
        double t = (p - 0.66) / 0.34;
        return {
            static_cast<uint8_t>(0),
            static_cast<uint8_t>(255),
            static_cast<uint8_t>(100 * t)
        };
    }
}

std::string Heatmap::markov(const std::vector<std::vector<double>>& trans,
                            const std::vector<std::string>& states) {
    std::ostringstream out;

    if (trans.size() != 3 || trans[0].size() != 3) {
        return "  Invalid transition matrix (expected 3x3)\n";
    }

    // Header
    out << "  " << BOLD << "Transition Matrix:" << RESET << "\n\n";
    out << "  " << DIM << "From \\ To" << RESET << "   ";

    // Column headers
    for (const auto& s : states) {
        out << std::setw(8) << std::left << s.substr(0, 8);
    }
    out << "\n\n";

    // Rows
    for (int r = 0; r < 3; ++r) {
        out << "  " << DIM << std::setw(10) << std::left << states[r] << RESET;
        for (int c = 0; c < 3; ++c) {
            double val = trans[r][c];
            RGB col = prob_to_color(val);
            int pct = static_cast<int>(val * 100);

            out << bg(col) << fg({0, 0, 0}) << std::setw(8) << pct << "%" << RESET << " ";
        }
        out << "\n";
    }

    out << "\n";
    return out.str();
}

} // namespace Axiom::UI
