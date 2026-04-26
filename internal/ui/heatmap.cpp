#include "ui/heatmap.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <cassert>

namespace Axiom::UI {

// ─── ANSI escape helpers (Delegated to UI) ────────────────────────────────────

std::string Heatmap::fg(RGB c) { return Axiom::UI::fg(c); }
std::string Heatmap::bg(RGB c) { return Axiom::UI::bg(c); }
std::string Heatmap::reset() { return Axiom::UI::RESET; }

// ─── Palette definitions ──────────────────────────────────────────────────────

static std::vector<RGB> palette_stops(Palette p, const std::vector<RGB>& custom) {
    switch (p) {
    case Palette::RdYlGn:
        return {{165,0,38},{215,48,39},{244,109,67},{253,174,97},
                {254,224,139},{255,255,191},{217,239,139},
                {166,217,106},{102,189,99},{26,152,80},{0,104,55}};
    case Palette::RdBu:
        return {{103,0,31},{178,24,43},{214,96,77},{244,165,130},
                {253,219,199},{247,247,247},{209,229,240},
                {146,197,222},{67,147,195},{33,102,172},{5,48,97}};
    case Palette::Plasma:
        return {{13,8,135},{84,2,163},{139,10,165},{185,50,137},
                {219,92,104},{244,136,73},{254,188,43},{240,249,33}};
    case Palette::Viridis:
        return {{68,1,84},{72,40,120},{62,83,160},{49,104,142},
                {38,130,142},{31,158,137},{53,183,121},{110,206,88},
                {181,222,43},{253,231,37}};
    case Palette::Inferno:
        return {{0,0,4},{40,11,84},{101,21,110},{159,42,99},
                {212,72,66},{245,125,21},{252,193,19},{252,255,164}};
    case Palette::Monochrome:
        return {{255,255,255},{0,0,0}};
    case Palette::Custom:
        return custom.empty() ? std::vector<RGB>{{255,255,255},{0,0,0}} : custom;
    }
    return {{255,255,255},{0,0,0}};
}

RGB Heatmap::interpolate_palette(double t) const {
    t = std::clamp(t, 0.0, 1.0);
    auto stops = palette_stops(opts_.palette, opts_.custom_stops);
    if (stops.size() == 1) return stops[0];

    double scaled = t * (stops.size() - 1);
    int    lo     = static_cast<int>(scaled);
    double frac   = scaled - lo;
    if (lo >= static_cast<int>(stops.size()) - 1)
        return stops.back();

    RGB a = stops[lo], b = stops[lo + 1];
    return {
        static_cast<uint8_t>(a.r + frac * (b.r - a.r)),
        static_cast<uint8_t>(a.g + frac * (b.g - a.g)),
        static_cast<uint8_t>(a.b + frac * (b.b - a.b)),
    };
}

// ─── Constructors ─────────────────────────────────────────────────────────────

Heatmap::Heatmap(std::vector<std::vector<double>> data,
                 std::vector<std::string>         row_labels,
                 std::vector<std::string>         col_labels,
                 HeatmapOptions                   opts)
    : row_labels_(std::move(row_labels))
    , col_labels_(std::move(col_labels))
    , opts_(std::move(opts))
{
    for (auto& row : data) {
        std::vector<HeatCell> r;
        r.reserve(row.size());
        for (double v : row) r.push_back({v, {}, false});
        cells_.push_back(std::move(r));
    }
    resolve_range();
}

Heatmap::Heatmap(std::vector<std::vector<HeatCell>> cells,
                 std::vector<std::string>            row_labels,
                 std::vector<std::string>            col_labels,
                 HeatmapOptions                      opts)
    : cells_(std::move(cells))
    , row_labels_(std::move(row_labels))
    , col_labels_(std::move(col_labels))
    , opts_(std::move(opts))
{
    resolve_range();
}

// ─── Range resolution ─────────────────────────────────────────────────────────

void Heatmap::resolve_range() {
    double mn =  1e308, mx = -1e308;
    for (auto& row : cells_)
        for (auto& c : row) {
            mn = std::min(mn, c.value);
            mx = std::max(mx, c.value);
        }

    vmin_ = opts_.vmin.value_or(mn);
    vmax_ = opts_.vmax.value_or(mx);

    if (opts_.symmetric) {
        double abs_max = std::max(std::abs(vmin_), std::abs(vmax_));
        vmin_ = -abs_max;
        vmax_ =  abs_max;
    }

    if (std::abs(vmax_ - vmin_) < 1e-12) { vmin_ -= 1.0; vmax_ += 1.0; }
}

RGB Heatmap::value_to_rgb(double v) const {
    double t = (v - vmin_) / (vmax_ - vmin_);
    return interpolate_palette(std::clamp(t, 0.0, 1.0));
}

// ─── Value formatting ─────────────────────────────────────────────────────────

std::string Heatmap::format_value(double v) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(opts_.value_decimals) << v;
    return oss.str();
}

// ─── Perceived luminance ──────────────────────────────────────────────────────

static double luminance(RGB c) {
    auto lin = [](uint8_t ch) -> double {
        double x = ch / 255.0;
        return x <= 0.04045 ? x / 12.92 : std::pow((x + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * lin(c.r) + 0.7152 * lin(c.g) + 0.0722 * lin(c.b);
}

static RGB contrast_fg(RGB bg_color) {
    return luminance(bg_color) > 0.35 ? RGB{20,20,20} : RGB{235,235,235};
}

// ─── Colorbar ─────────────────────────────────────────────────────────────────

std::string Heatmap::render_colorbar(int width) const {
    std::ostringstream out;
    out << "  ";
    for (int x = 0; x < width; ++x) {
        double t = static_cast<double>(x) / (width - 1);
        RGB    c = interpolate_palette(t);
        out << bg(c) << "  " << reset();
    }
    out << "\n";

    std::string lo  = format_value(vmin_);
    std::string mid = format_value((vmin_ + vmax_) / 2.0);
    std::string hi  = format_value(vmax_);

    int bar_chars = width * 2;
    out << "  " << lo;
    int mid_pos = bar_chars / 2 - static_cast<int>(mid.size()) / 2 - static_cast<int>(lo.size());
    for (int i = 0; i < mid_pos; ++i) out << ' ';
    out << mid;
    int hi_pos = bar_chars - static_cast<int>(lo.size()) - mid_pos - static_cast<int>(mid.size()) - static_cast<int>(hi.size());
    for (int i = 0; i < hi_pos; ++i) out << ' ';
    out << hi << "\n";

    return out.str();
}

// ─── Core renderer ────────────────────────────────────────────────────────────

std::string Heatmap::to_string() const {
    if (cells_.empty()) return "(empty heatmap)\n";

    const int   nrows      = static_cast<int>(cells_.size());
    const int   ncols      = static_cast<int>(cells_[0].size());
    const int   cw         = opts_.cell_width;
    const bool  unicode    = opts_.use_unicode_box;
    const bool  has_rows   = opts_.show_axes && !row_labels_.empty();
    const bool  has_cols   = opts_.show_axes && !col_labels_.empty();

    int rl_width = 0;
    if (has_rows) {
        for (auto& l : row_labels_)
            rl_width = std::max(rl_width, static_cast<int>(l.size()));
        rl_width += 2;
    }

    const int   total_bar_cols = ncols * (cw / 2);
    std::ostringstream out;

    if (!opts_.title.empty()) {
        int total_w = rl_width + ncols * cw;
        int pad     = std::max(0, (total_w - static_cast<int>(opts_.title.size())) / 2);
        out << std::string(pad, ' ') << "\033[1m" << opts_.title << "\033[0m\n\n";
    }

    if (has_cols) {
        out << std::string(rl_width, ' ');
        for (int c = 0; c < ncols; ++c) {
            std::string lbl = (c < static_cast<int>(col_labels_.size())) ? col_labels_[c] : "";
            if (static_cast<int>(lbl.size()) > cw - 1) lbl = lbl.substr(0, cw - 1);
            int lpad = (cw - static_cast<int>(lbl.size())) / 2;
            int rpad = cw - static_cast<int>(lbl.size()) - lpad;
            out << std::string(lpad, ' ') << "\033[2m" << lbl << "\033[0m" << std::string(rpad, ' ');
        }
        out << "\n";
    }

    auto rep = [](const std::string& s, int n) {
        std::string r;
        for (int i = 0; i < n; ++i) r += s;
        return r;
    };

    {
        out << std::string(rl_width, ' ');
        if (unicode) {
            out << "┌";
            for (int c = 0; c < ncols; ++c) {
                out << rep("─", cw);
                if (c < ncols - 1) out << "┬";
            }
            out << "┐\n";
        } else {
            out << "+";
            for (int c = 0; c < ncols; ++c) out << rep("-", cw) << "+";
            out << "\n";
        }
    }

    for (int r = 0; r < nrows; ++r) {
        for (int line = 0; line < opts_.cell_height; ++line) {
            if (has_rows) {
                std::string lbl = (r < static_cast<int>(row_labels_.size())) ? row_labels_[r] : "";
                if (line == opts_.cell_height / 2) {
                    if (static_cast<int>(lbl.size()) > rl_width - 2) lbl = lbl.substr(0, rl_width - 2);
                    int rpad = rl_width - static_cast<int>(lbl.size()) - 1;
                    out << "\033[2m" << lbl << "\033[0m" << std::string(rpad, ' ');
                } else {
                    out << std::string(rl_width, ' ');
                }
            }

            if (unicode) out << "│"; else out << "|";

            for (int c = 0; c < ncols; ++c) {
                const HeatCell& cell = cells_[r][c];
                RGB bg_col = value_to_rgb(cell.value);
                RGB text_col = contrast_fg(bg_col);
                bool show_text = (line == opts_.cell_height / 2) && opts_.show_values;

                std::string val_str;
                if (show_text) {
                    val_str = cell.label.empty() ? format_value(cell.value) : cell.label;
                    if (static_cast<int>(val_str.size()) > cw - 1) val_str = val_str.substr(0, cw - 1);
                }

                out << bg(bg_col) << fg(text_col);
                if (show_text && !val_str.empty()) {
                    int lpad = (cw - static_cast<int>(val_str.size())) / 2;
                    int rpad = cw - static_cast<int>(val_str.size()) - lpad;
                    out << std::string(lpad, ' ') << val_str << std::string(rpad, ' ');
                } else {
                    out << std::string(cw, ' ');
                }
                out << reset();

                if (c < ncols - 1) { if (unicode) out << "│"; else out << "|"; }
            }
            if (unicode) out << "│\n"; else out << "|\n";
        }

        if (r < nrows - 1) {
            out << std::string(rl_width, ' ');
            if (unicode) {
                out << "├";
                for (int c = 0; c < ncols; ++c) {
                    out << rep("─", cw);
                    if (c < ncols - 1) out << "┼";
                }
                out << "┤\n";
            } else {
                out << "+";
                for (int c = 0; c < ncols; ++c) out << rep("-", cw) << "+";
                out << "\n";
            }
        }
    }

    {
        out << std::string(rl_width, ' ');
        if (unicode) {
            out << "└";
            for (int c = 0; c < ncols; ++c) {
                out << rep("─", cw);
                if (c < ncols - 1) out << "┴";
            }
            out << "┘\n";
        } else {
            out << "+";
            for (int c = 0; c < ncols; ++c) out << rep("-", cw) << "+";
            out << "\n";
        }
    }

    if (opts_.show_colorbar) {
        out << "\n";
        out << render_colorbar(total_bar_cols);
    }

    out << "\n";
    return out.str();
}

void Heatmap::render() const { std::cout << to_string() << std::flush; }

Heatmap Heatmap::correlation(std::vector<std::vector<double>> corr, std::vector<std::string> labels, HeatmapOptions opts) {
    opts.palette = Palette::RdBu; opts.symmetric = true; opts.vmin = -1.0; opts.vmax = 1.0;
    int n = static_cast<int>(corr.size());
    std::vector<std::vector<HeatCell>> cells(n, std::vector<HeatCell>(n));
    for (int r = 0; r < n; ++r) for (int c = 0; c < n; ++c) {
        cells[r][c].value = corr[r][c]; cells[r][c].highlighted = (r == c);
    }
    return Heatmap(std::move(cells), labels, labels, std::move(opts));
}

Heatmap Heatmap::returns(std::vector<std::vector<double>> rets, std::vector<std::string> row_labels, std::vector<std::string> col_labels, HeatmapOptions opts) {
    opts.palette = Palette::RdYlGn; opts.symmetric = true;
    return Heatmap(std::move(rets), std::move(row_labels), std::move(col_labels), std::move(opts));
}

Heatmap Heatmap::markov(std::vector<std::vector<double>> trans, std::vector<std::string> states, HeatmapOptions opts) {
    opts.palette = Palette::Viridis; opts.vmin = 0.0; opts.vmax = 1.0;
    return Heatmap(std::move(trans), states, states, std::move(opts));
}

} // namespace Axiom::UI
