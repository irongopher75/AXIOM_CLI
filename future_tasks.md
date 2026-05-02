# Future Tasks for Axiom CLI

## Phase 1 — Persistence Layer (Weeks 1–2)
- [ ] **Replace in-memory cache with DuckDB**: Embed DuckDB for columnar compression and SQL capabilities. Schema target: `bars(symbol TEXT, ts TIMESTAMP, open DOUBLE, high DOUBLE, low DOUBLE, close DOUBLE, volume BIGINT, source TEXT, fetched_at TIMESTAMP)`.
- [ ] **Define a typed `AxiomError` enum**: Taxonomy for explicit failure modes (NetworkTimeout, RateLimited, AuthFailed, MarketClosed, SymbolDelisted, PartialPayload, ClockSkew, CacheCorruption, ConfigSaveFailure).

## Phase 4 — Documentation and Policy
- [ ] **Create `CONTRIBUTING.md`**: Detail local build instructions, test suite execution, style conventions (`clang-format`), PR guidelines, and actively desired contributions.
- [ ] **Establish Security Policy**: Document API key storage via `ConfigService`, threat models, and optional `AXIOM_API_KEY_FINNHUB` environment variable support.

## Deliberate Deferrals (Post-v1)
- [ ] Plugin API
- [ ] Windows support (TrueColor ANSI, `notify-send`, `linenoise` REPL compatibility)
- [ ] Multi-asset classes (crypto, futures, options)
- [ ] Python scripting layer
- [ ] Web UI / API server mode
