#pragma once
#include <string>
#include <variant>

namespace Axiom {

enum class DataError {
    None,
    RateLimit,
    NetworkFailure,
    ParseError,
    TickerNotFound,
    InsufficientData,
    ConfigError,
    AuthError,
    InternalError
};

inline std::string error_to_string(DataError err) {
    switch (err) {
        case DataError::RateLimit: return "Rate limit exceeded";
        case DataError::NetworkFailure: return "Network connection failed";
        case DataError::ParseError: return "Failed to parse data";
        case DataError::TickerNotFound: return "Ticker not found";
        case DataError::InsufficientData: return "Insufficient historical data";
        case DataError::ConfigError: return "Configuration error";
        case DataError::AuthError: return "Authentication failed";
        case DataError::InternalError: return "Internal engine error";
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
