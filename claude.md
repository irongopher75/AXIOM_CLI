## Axiom CLI — Full Proof Plan for Public Open Source Release

---

### The Core Constraint Everything Else Flows From

You have in-memory-only caching and two free data sources (Yahoo Finance, Finnhub). Every architectural decision in this plan is made with that reality in mind. The plan does not assume you'll upgrade to a paid data feed. It does not assume a server. The entire system runs locally, owned by the user, with their own API keys — which is the product's identity and its strongest competitive angle.

---

### Phase 1 — Persistence Layer (Weeks 1–2)

This is the most important phase and the one most people skip. Don't.

**1.1 Replace in-memory cache with DuckDB**

DuckDB is a single embedded library with no server process. You link it into the binary the same way you'd link SQLite. The payoff is enormous: columnar compression means 30 days of 1-minute OHLCV bars for 50 symbols fits in under 100MB. You get SQL for free. And every downstream feature — screener, backtester, alerts — runs queries directly against DuckDB instead of requiring a separate data pipeline.

The schema to target from day one:

```sql
bars(symbol TEXT, ts TIMESTAMP, open DOUBLE, high DOUBLE, low DOUBLE, close DOUBLE, volume BIGINT, source TEXT, fetched_at TIMESTAMP)
```

Partition by symbol and date. Add a retention policy that auto-expires bars older than your configured window (90 days is a sensible default). The `source` column matters — when you add multi-source fallback in Phase 3, you'll want to know which bars came from Yahoo vs Finnhub for auditing.

**1.2 Define a typed AxiomError enum — now, before it gets painful**

Right now you almost certainly have a mix of `std::cerr` outputs, return codes, and exceptions. Before the codebase gets larger, define a typed error taxonomy. Every failure mode in a financial terminal is distinct and needs to be handled differently:

```cpp
enum class AxiomError {
    NetworkTimeout,
    RateLimited,          // 429 — back off and retry
    AuthFailed,           // API key invalid or expired
    MarketClosed,         // Not an error, just a state
    SymbolDelisted,       // Permanent — purge from cache
    PartialPayload,       // Truncated JSON — discard and re-fetch
    ClockSkew,            // Bar timestamp out of expected range
    CacheCorruption,      // DuckDB read returned unexpected schema
    ConfigSaveFailure,    // Already identified in you
};
```

Do not write a wall of feature bullets. Show, don't list.

**4.4 CONTRIBUTING.md**

Cover: how to build locally, how to run the test suite, code style conventions (clang-format config committed to the repo), how to submit a PR, and what kinds of contributions you're actively looking for. Be specific about the last point — if you want people to add technical indicators or data source adapters, say so explicitly. Vague contribution guides get ignored.

**4.5 Security policy**

Because users are storing API keys locally via `ConfigService`, document how keys are stored (plaintext in `~/.axiom/config`? encrypted?), what the threat model is, and what users should do if they suspect a key leak. If you're storing keys in plaintext right now, consider adding an optional `AXIOM_API_KEY_FINNHUB` environment variable as an alternative — many developers are more comfortable with env vars than config files for secrets.

---

### What to Deliberately Defer Past v1

These are good ideas that will hurt you if you build them now:

- **Plugin API.** Your own feature surface isn't stable enough to design a good plugin interface yet. Build it after you've used the terminal heavily yourself for a month.
- **Windows support.** The TrueColor ANSI rendering, the `notify-send` alerts, and the linenoise REPL all have Windows compatibility issues. It's a separate project. Defer it.
- **Multi-asset classes (crypto, futures, options).** Each asset class has different data schemas, different market hours, different quoting conventions. Go deep on equities first.
- **Python scripting layer.** Tempting for backtesting strategy definitions, but embedding a Python interpreter adds significant complexity and a heavy dependency. This belongs in the plugin API milestone.
- **Web UI / API server mode.** You are building a terminal. Stay a terminal.

---

### The Single Number to Keep In Mind

Eight weeks to a public release worth announcing. That is achievable. The risk to the timeline is almost entirely in Phase 2 — specifically, the ring buffer architecture and the data integrity work. If you find yourself cutting corners on the quality gate to get to features faster, you are building the wrong thing. The data layer is the product. Everything else is UI on top of it.
