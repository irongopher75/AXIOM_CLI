# Axiom CLI — Institutional Intelligence Terminal

A high-performance C++20 terminal for quantitative market analysis, regime forecasting, and technical profiling.

![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?style=flat-square&logo=c%2B%2B)
![License](https://img.shields.io/badge/License-MIT-emerald?style=flat-square)
![Platform](https://img.shields.io/badge/Platform-macOS%20%7C%20Linux-lightgrey?style=flat-square)

---

## 🏛️ Institutional UI
Axiom transforms your standard shell into a professional financial workstation using 24-bit TrueColor (ANSI) and structured layout engines.

- **Panel-Driven Layout**: High-density data cards with precise padding and alignment.
*   **Regime Pills**: Instant visual status for Bullish/Bearish regimes, RSI conditions, and trend persistence.
*   **TrueColor Heatmaps**: Statistical transition matrices rendered with perceptually uniform color palettes (Viridis, Plasma).
*   **Categorized Technical Profile**: 50+ indicators grouped by Trend, Momentum, and Volatility.

## 🧠 Core Analysis Engine
- **Markov MLE Forecasting**: Maximum Likelihood Estimation engine that models market state transitions and predicts future regimes with statistical confidence intervals.
- **Advanced Indicator Suite**: Implements high-order primitives including DEMA, KAMA, Hurst Exponents, Linear Regression (Slope/R2), and Volume-Weighted Average Price (VWAP).
- **Intelligent Ticker Resolution**: Hybrid search engine combining exact ticker matching with fuzzy name-based resolution (Exact > Prefix > Fuzzy).
- **Tiered Data Pipeline**: Multi-word query support with daily-auto-refresh disk caching for zero-latency retrieval.

---

## 🚀 Installation

### Prerequisites
- `libcurl`
- `nlohmann/json` (included)
- C++20 compliant compiler (clang++ or g++)

### Build from Source
```bash
make clean && make
sudo make install
```

---

## ⌨️ Command Reference

Start the terminal with:
```bash
axiom
```

| Command | Action | Example |
| :--- | :--- | :--- |
| `predict <symbol>` | Run Markov Chain regime forecasting | `p AAPL` |
| `analyze <symbol>` | Run full technical indicator suite | `a TSLA` |
| `<symbol>` | Execute combined intelligence report | `NVDA` |
| `clear` | Wipe terminal screen | `clear` |
| `help` | Show command reference | `help` |
| `exit` | Terminate session | `quit` |

---

## ⚙️ Configuration
Axiom requires a **Polygon.io** API key to fetch real-time and historical bars. 

You can set your key globally by exporting it in your `.zshrc` or `.bashrc`:
```bash
export POLYGON_API_KEY="your_api_key_here"
```
*Note: If no environment variable is found, Axiom will attempt to use the built-in fallback key.*

---

## 📁 Project Structure
- `src/cpp/engine/`: Core analytics (Indicators, Markov MLE, Data Pipeline).
- `src/cpp/ui/`: Institutional UI layer (Panels, Heatmaps, TrueColor helpers).
- `src/cpp/core/`: Internal domain types and state management.
- `~/.axiom/cache/`: Persistent local data storage.

---
*Built for quantitative analysts and terminal power users.*
