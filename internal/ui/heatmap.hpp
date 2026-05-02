#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "ui/ui.hpp"

namespace Axiom::UI {

// Simplified heatmap for Markov transition matrix display.
// Renders a 3x3 matrix with basic color coding (low/med/high probability).
class Heatmap {
public:
    // Render Markov transition matrix with state labels.
    // trans: 3x3 matrix where trans[i][j] = probability of transitioning from state i to state j.
    // states: labels for rows/columns (e.g., {"Bearish", "Neutral", "Bullish"}).
    static std::string markov(const std::vector<std::vector<double>>& trans,
                              const std::vector<std::string>& states);

private:
    // Get color for probability value (0.0-1.0).
    static RGB prob_to_color(double p);
};

} // namespace Axiom::UI
