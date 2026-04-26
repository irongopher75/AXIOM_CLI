#pragma once
#include <vector>
#include <string>
#include <optional>
#include <functional>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include "ui/ui.hpp"

namespace Axiom::UI {

// ─── Color palettes ───────────────────────────────────────────────────────────

enum class Palette {
    RdYlGn,     // Red → Yellow → Green  (P&L, returns)
    RdBu,       // Red → White → Blue    (correlation, z-scores)
    Plasma,     // Dark purple → yellow  (heat/intensity)
    Viridis,    // Purple → teal → yellow (perceptually uniform)
    Inferno,    // Black → red → yellow  (volatility, risk)
    Monochrome, // White → black
    Custom,     // User-supplied stops
};

// ─── Cell annotation ──────────────────────────────────────────────────────────

struct HeatCell {
    double      value{0.0};
    std::string label{};       // Overlaid text (e.g. "3.2%"). Empty = auto.
    bool        highlighted{false};  // Draws a border around this cell.
};

// ─── Layout options ───────────────────────────────────────────────────────────

struct HeatmapOptions {
    // Dimensions
    int cell_width{8};         // Terminal columns per cell
    int cell_height{2};        // Terminal rows per cell (2 = half-block trick)

    // Color
    Palette palette{Palette::RdYlGn};
    std::vector<RGB> custom_stops{};  // Used when palette == Custom

    // Range — auto-computed from data if not set
    std::optional<double> vmin{};
    std::optional<double> vmax{};
    bool symmetric{false};     // Force range to be [-max, +max] (good for returns)

    // Labels
    bool show_values{true};    // Print value inside each cell
    int  value_decimals{2};    // Decimal places for auto value labels
    bool show_colorbar{true};  // Legend strip below the heatmap
    bool show_axes{true};      // Row/column header labels

    // Title
    std::string title{};

    // Border style
    bool use_unicode_box{true}; // ╔═╗ etc. vs plain ASCII +--+
};

// ─── Main renderer ────────────────────────────────────────────────────────────

class Heatmap {
public:
    // rows × cols grid of values.  Row/col labels are optional.
    Heatmap(std::vector<std::vector<double>> data,
            std::vector<std::string>         row_labels = {},
            std::vector<std::string>         col_labels = {},
            HeatmapOptions                   opts       = {});

    // Rich version: supply pre-built HeatCells for custom labels / highlights.
    Heatmap(std::vector<std::vector<HeatCell>> cells,
            std::vector<std::string>           row_labels = {},
            std::vector<std::string>           col_labels = {},
            HeatmapOptions                     opts       = {});

    // Render to stdout.
    void render() const;

    // Render to a string (for piping / logging).
    std::string to_string() const;

    // ── Convenience factories ─────────────────────────────────────────────────

    // Correlation matrix (symmetric range, RdBu palette, diagonal = 1).
    static Heatmap correlation(std::vector<std::vector<double>> corr,
                               std::vector<std::string>         labels = {},
                               HeatmapOptions                   opts   = {});

    // Return heatmap: rows = assets/strategies, cols = time periods.
    static Heatmap returns(std::vector<std::vector<double>> rets,
                           std::vector<std::string>         row_labels = {},
                           std::vector<std::string>         col_labels = {},
                           HeatmapOptions                   opts       = {});

    // Markov transition matrix: rows = from-state, cols = to-state.
    static Heatmap markov(std::vector<std::vector<double>> trans,
                          std::vector<std::string>         states = {},
                          HeatmapOptions                   opts   = {});

private:
    std::vector<std::vector<HeatCell>> cells_;
    std::vector<std::string>           row_labels_;
    std::vector<std::string>           col_labels_;
    HeatmapOptions                     opts_;

    // Resolved range after construction
    double vmin_{}, vmax_{};

    void   resolve_range();
    RGB    value_to_rgb(double v) const;
    RGB    interpolate_palette(double t) const;  // t in [0,1]
    std::string render_colorbar(int width) const;
    std::string format_value(double v) const;

    // ANSI helpers
    static std::string fg(RGB c);
    static std::string bg(RGB c);
    static std::string reset();
};

} // namespace Axiom::UI
