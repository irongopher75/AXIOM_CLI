#pragma once
#include <string>
#include <variant>

namespace Axiom {

enum class AxiomError {
    None,
    NetworkTimeout,
    RateLimited,          // 429 — back off and retry
    AuthFailed,           // API key invalid or expired
    MarketClosed,         // Not an error, just a state
    SymbolDelisted,       // Permanent — purge from cache
    PartialPayload,       // Truncated JSON — discard and re-fetch
    ClockSkew,            // Bar timestamp out of expected range
    CacheCorruption,      // DuckDB read returned unexpected schema
    ConfigSaveFailure,
    TickerNotFound,
    InsufficientData,
    ConfigError,
    ParseError,
    InternalError
};

inline std::string error_to_string(AxiomError err) {
    switch (err) {
        case AxiomError::NetworkTimeout: return "Network timeout";
        case AxiomError::RateLimited: return "Rate limit exceeded";
        case AxiomError::AuthFailed: return "Authentication failed";
        case AxiomError::MarketClosed: return "Market closed";
        case AxiomError::SymbolDelisted: return "Symbol delisted";
        case AxiomError::PartialPayload: return "Partial payload received";
        case AxiomError::ClockSkew: return "Clock skew detected";
        case AxiomError::CacheCorruption: return "Cache corruption detected";
        case AxiomError::ConfigSaveFailure: return "Config save failure";
        case AxiomError::TickerNotFound: return "Ticker not found";
        case AxiomError::InsufficientData: return "Insufficient historical data";
        case AxiomError::ConfigError: return "Configuration error";
        case AxiomError::ParseError: return "Failed to parse data";
        case AxiomError::InternalError: return "Internal engine error";
        case AxiomError::None: return "No error";
        default: return "Unknown error";
    }
}

// ---------------------------------------------------------------------------
// Expected<T, E> - A lightweight backport of C++23 std::expected
// ---------------------------------------------------------------------------
template <typename T, typename E>
class Expected {
private:
    std::variant<T, E> storage;

public:
    Expected(const T& val) : storage(val) {}
    Expected(T&& val) : storage(std::move(val)) {}
    Expected(const E& err) : storage(err) {}
    Expected(E&& err) : storage(std::move(err)) {}

    bool has_value() const { return std::holds_alternative<T>(storage); }
    explicit operator bool() const { return has_value(); }

    T& value() { return std::get<T>(storage); }
    const T& value() const { return std::get<T>(storage); }
    
    T& operator*() { return value(); }
    const T& operator*() const { return value(); }
    T* operator->() { return &value(); }
    const T* operator->() const { return &value(); }

    E& error() { return std::get<E>(storage); }
    const E& error() const { return std::get<E>(storage); }
};

} // namespace Axiom
