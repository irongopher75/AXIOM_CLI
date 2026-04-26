# AXIOM CLI — Institutional Intelligence Terminal

A high-performance, professional-grade C++20 command-line tool for quantitative market analysis, regime forecasting, and technical profiling.

![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?style=flat-square&logo=c%2B%2B)
![Architecture](https://img.shields.io/badge/Architecture-Service--Layer-emerald?style=flat-square)
![License](https://img.shields.io/badge/License-MIT-blue?style=flat-square)

---

## 🏛️ What is Axiom?
Axiom is a data processing and analysis CLI designed for quantitative analysts and terminal power users. It transforms raw market data into institutional-grade insights using advanced statistical models and a high-density UI.

### Key Features
- **Markov MLE Forecasting**: Predict market regime transitions (Bullish/Bearish/Neutral) with statistical confidence.
- **50+ Technical Indicators**: From standard SMA/RSI to high-order primitives like Hurst Exponents and DEMA.
- **Service-Oriented Architecture**: Decoupled engine, data, and UI layers for maximum scalability.
- **TrueColor Terminal UI**: High-density data cards and transition matrix heatmaps rendered in 24-bit color.

---

## 🚀 Installation

### Prerequisites
- `libcurl`
- C++20 compliant compiler (`clang++` or `g++`)

### Build from Source
```bash
# Clone and build
make clean && make

# Install globally
sudo make install
```

---

## ⌨️ Usage

Axiom follows a modern CLI command structure:
`axiom [command] [symbol] [flags]`

### Core Commands
| Command | Action | Example |
| :--- | :--- | :--- |
| `analyze` | Run full technical indicator suite | `axiom analyze NVDA` |
| `predict` | Execute Markov regime forecasting | `axiom predict AAPL` |
| `fetch`   | Pre-fetch and cache data locally | `axiom fetch TSLA` |
| `repl`    | Enter interactive terminal mode | `axiom repl` |
| `config`  | Manage API keys and settings | `axiom config` |

### Example Output
```bash
$ axiom analyze AAPL
✔ Found 252 records
✔ Trend: Bullish
✔ Top Indicator: RSI (62.4)
```

---

## 🏗️ Architecture
Axiom is built with a scalable, industry-standard directory structure:
- `cmd/`: CLI entry points and command dispatching.
- `internal/`: Private logic (Engine, Service Layer, UI Rendering).
- `pkg/`: Reusable packages (JSON parser, math utilities).
- `configs/`: Configuration management logic.

---

## ⚙️ Configuration
Axiom requires a **Polygon.io** API key. Set it via environment variable:
```bash
export POLYGON_API_KEY="your_api_key_here"
```

---

## 🗺️ Roadmap
- [ ] Multi-asset support (Crypto, Forex).
- [ ] Export to CSV/JSON format.
- [ ] Real-time websocket streaming.
- [ ] Custom plugin system for indicators.

---
*Built for the next generation of quantitative finance.*
