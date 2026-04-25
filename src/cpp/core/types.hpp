#pragma once
#include <cstdint>
#include <string>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace Axiom {

// --- FIXED POINT PRICE (4 DECIMALS) ---
class Price {
private:
    int64_t raw; // value * 10,000
    static constexpr int64_t SCALE = 10000;

public:
    Price() : raw(0) {}
    explicit Price(int64_t r) : raw(r) {}
    Price(double d) : raw(static_cast<int64_t>(std::round(d * SCALE))) {}

    static Price from_raw(int64_t r) { return Price(r); }
    int64_t get_raw() const { return raw; }
    double to_double() const { return static_cast<double>(raw) / SCALE; }

    Price operator+(const Price& other) const { return Price(raw + other.raw); }
    Price operator-(const Price& other) const { return Price(raw - other.raw); }
    Price operator*(double factor) const { return Price(static_cast<int64_t>(std::round(raw * factor))); }
    
    bool operator>(const Price& other) const { return raw > other.raw; }
    bool operator<(const Price& other) const { return raw < other.raw; }
    bool operator>=(const Price& other) const { return raw >= other.raw; }
    bool operator<=(const Price& other) const { return raw <= other.raw; }

    std::string str(int precision = 2) const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision) << to_double();
        return oss.str();
    }
};

} // namespace Axiom
