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
    InsufficientData
};

inline std::string error_to_string(DataError err) {
    switch (err) {
        case DataError::RateLimit: return "Rate limit exceeded";
        case DataError::NetworkFailure: return "Network connection failed";
        case DataError::ParseError: return "Failed to parse data";
        case DataError::TickerNotFound: return "Ticker not found";
        case DataError::InsufficientData: return "Insufficient historical data";
        default: return "Unknown error";
    }
}

template <typename T>
class Result {
private:
    std::variant<T, DataError> storage;

public:
    Result(T val) : storage(val) {}
    Result(DataError err) : storage(err) {}

    static Result<T> success(const T& val) { return Result<T>(val); }
    static Result<T> fail(DataError err) { return Result<T>(err); }

    bool has_value() const { return std::holds_alternative<T>(storage); }
    T& value() { return std::get<T>(storage); }
    const T& value() const { return std::get<T>(storage); }
    DataError error() const { return std::get<DataError>(storage); }
};

} // namespace Axiom
