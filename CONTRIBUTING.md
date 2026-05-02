# Contributing to Axiom

Thank you for your interest in contributing. Axiom is a compiled C++20 institutional terminal — contributions are expected to be precise, well-tested, and respectful of the existing architecture.

---

## Table of Contents

- [Building Locally](#building-locally)
- [Running the Test Suite](#running-the-test-suite)
- [Code Style](#code-style)
- [Project Structure](#project-structure)
- [Submitting a Pull Request](#submitting-a-pull-request)
- [What We're Looking For](#what-were-looking-for)
- [What We're Not Looking For](#what-were-not-looking-for)

---

## Building Locally

### Prerequisites

| Dependency | Version | Notes |
|---|---|---|
| C++ compiler | GCC ≥ 11 or Clang ≥ 14 | Must support C++20 |
| Make | Any modern version | |
| libcurl | ≥ 7.68 | `libcurl4-openssl-dev` on Ubuntu |
| SQLite | ≥ 3.35 | `libsqlite3-dev` on Ubuntu |
| linenoise | Bundled | No system install needed |

**Ubuntu / Debian:**
```bash
sudo apt install build-essential libcurl4-openssl-dev libsqlite3-dev
```

**macOS (Homebrew):**
```bash
brew install curl sqlite
```

### Build

```bash
git clone https://github.com/your-org/axiom.git
cd axiom
make
```

The binary is output to `./build/axiom`. There is no install step — you can symlink it or add `./build` to your `PATH`.

### Verify

```bash
./build/axiom --version
./build/axiom config set FINNHUB_API_KEY your_key_here
./build/axiom quote AAPL
```

---

## Running the Test Suite

```bash
make test
```

Tests are written with [doctest](https://github.com/doctest/doctest) and live in `tests/`. The suite covers core analysis logic and UI rendering. It must pass cleanly before any PR is considered.

To run a specific test file:
```bash
./build/test_main
./build/test_ui
```

If you add new functionality, add tests for it. PRs without tests for new logic will be asked to add them before merge.

---

## Code Style

Axiom uses `clang-format` for consistent formatting. A `.clang-format` file is committed to the root of the repository.

**Before committing:**
```bash
clang-format -i internal/**/*.cpp internal/**/*.h
```

Or install the pre-commit hook:
```bash
cp scripts/pre-commit .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

### Conventions

- **C++20** throughout. Use `std::optional`, `std::variant`, `std::string_view`, and structured bindings freely.
- **No raw owning pointers.** Use `std::unique_ptr` or `std::shared_ptr`. Raw pointers are acceptable for non-owning references.
- **Error handling via typed returns, not exceptions.** Functions that can fail return `std::expected<T, AxiomError>` (or a `std::optional<T>` for simpler cases). Do not throw.
- **No silent failures.** Every error path must resolve to a named `AxiomError` and be logged. `std::cerr` with a raw string is not acceptable in merged code.
- **ANSI output and JSON output are mutually exclusive per invocation.** Never mix colored output with JSON. The `--json` flag must produce clean, machine-readable output.
- **No STL headers pulled in through other headers.** Include explicitly what you use.

---

## Project Structure

```
axiom/
├── internal/
│   ├── core/        # Types, AxiomError, config primitives
│   ├── engine/      # DataEngine (SQLite), AnalysisEngine
│   ├── service/     # ConfigService, AnalysisService
│   └── ui/          # ANSI rendering, heatmap, REPL
├── tests/
│   ├── test_main.cpp
│   └── test_ui.cpp
├── Makefile
└── main.cpp
```

When adding a new feature, respect this boundary. Business logic goes in `engine/` or `service/`. Rendering goes in `ui/`. `core/` is for shared primitives only — do not add feature logic there.

---

## Submitting a Pull Request

1. **Fork the repository** and create a branch from `main`. Branch names should be descriptive: `feat/rsi-indicator`, `fix/config-save-silent-failure`, `docs/contributing`.
2. **Make your changes.** Keep commits focused — one logical change per commit.
3. **Run the test suite.** `make test` must pass cleanly.
4. **Run clang-format.** All changed files must be formatted before submitting.
5. **Open a PR against `main`.** Fill in the PR template completely. PRs with empty descriptions will not be reviewed.
6. **Expect feedback.** Code review is thorough. Respond to comments directly — don't just push changes without acknowledging the discussion.

### PR Checklist

- [ ] `make` produces a clean build with no warnings
- [ ] `make test` passes
- [ ] New logic has corresponding tests
- [ ] clang-format applied to all changed files
- [ ] No raw `std::cerr` error paths — all failures use `AxiomError`
- [ ] JSON output mode unaffected if you touched rendering code

---

## What We're Looking For

These are the contribution areas that are most valuable right now:

**Technical indicators** (`internal/engine/analysis_engine`)
New indicators added to the analysis engine. Each indicator must: accept a `std::vector<double>` of closes (or OHLCV where appropriate), return a typed result, and have doctest coverage. Good candidates: ATR, Bollinger Bands, VWAP, OBV, Stochastic.

**Data source adapters** (`internal/service/`)
Adapters for new free-tier data sources behind the existing `DataSource` interface. Must implement `write_bar`, `query_bars`, and `flush`. Must handle the source's specific error modes (rate limits, empty results, malformed payloads) and map them to `AxiomError` values.

**Platform build verification**
Confirmed clean builds on platforms not yet in CI — particularly Alpine Linux and older macOS versions. File an issue with your platform details and build output.

**Test coverage**
Additional doctest cases for edge conditions in existing code — particularly around SQLite write failures, malformed API responses, and REPL input edge cases.

---

## What We're Not Looking For

To keep the project focused, please do not open PRs for the following without prior discussion in an issue:

- Windows support
- Crypto or futures asset classes
- A web UI or HTTP API server mode
- Python scripting or strategy DSL
- A plugin system

These may be on the roadmap. They are not in scope for v1.

---

## Questions

Open an issue tagged `question`. Do not DM maintainers directly.
