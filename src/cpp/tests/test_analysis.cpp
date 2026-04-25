//
// test_analysis.cpp
// Tests for AnalysisEngine logic using synthetic price series.
// No network calls — feeds data directly into the computation functions.
//

#include <cassert>
#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>

#include "core/types.hpp"
#include "core/buffers.hpp"

using namespace Axiom;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static bool approx(double a, double b, double tol = 0.01) {
    return std::fabs(a - b) < tol;
}

// Reimport the pure math from analysis_engine without the network layer.
// These functions are extracted verbatim so they can be tested in isolation.

static double compute_rsi(const std::vector<Price>& px) {
    if (px.size() <= 14) return 50.0;
    double gain = 0.0, loss = 0.0;
    for (size_t i = px.size() - 14; i < px.size(); ++i) {
        double d = px[i].to_double() - px[i-1].to_double();
        if (d >= 0.0) gain += d; else loss -= d;
    }
    if (gain == 0.0 && loss == 0.0) return 50.0;
    return (loss == 0.0) ? 100.0 : 100.0 - (100.0 / (1.0 + gain / loss));
}

static Price compute_sma50(const std::vector<Price>& px) {
    CircularBuffer<Price, 50> buf;
    for (auto& p : px) buf.push(p);
    return Price(buf.avg());
}

// ---------------------------------------------------------------------------
// Test: SMA-50 on a flat series should equal that price
// ---------------------------------------------------------------------------
static void test_sma_flat() {
    std::vector<Price> prices(60, Price(100.0));
    Price sma = compute_sma50(prices);
    assert(approx(sma.to_double(), 100.0) && "SMA flat series");
    std::cout << "  PASS  sma_flat\n";
}

// ---------------------------------------------------------------------------
// Test: SMA-50 on rising series should lag behind current price
// ---------------------------------------------------------------------------
static void test_sma_rising() {
    std::vector<Price> prices;
    for (int i = 1; i <= 60; ++i) prices.emplace_back(static_cast<double>(i));
    Price sma = compute_sma50(prices);
    // Last 50 values: 11..60, mean = 35.5
    assert(approx(sma.to_double(), 35.5, 0.1) && "SMA rising series");
    assert(sma < prices.back() && "SMA lags current price");
    std::cout << "  PASS  sma_rising\n";
}

// ---------------------------------------------------------------------------
// Test: RSI on a purely rising series should approach 100
// ---------------------------------------------------------------------------
static void test_rsi_pure_up() {
    std::vector<Price> prices;
    for (int i = 1; i <= 30; ++i) prices.emplace_back(static_cast<double>(i));
    double rsi = compute_rsi(prices);
    assert(rsi > 90.0 && "RSI pure up series > 90");
    std::cout << "  PASS  rsi_pure_up  (rsi=" << rsi << ")\n";
}

// ---------------------------------------------------------------------------
// Test: RSI on a purely falling series should approach 0
// ---------------------------------------------------------------------------
static void test_rsi_pure_down() {
    std::vector<Price> prices;
    for (int i = 30; i >= 1; --i) prices.emplace_back(static_cast<double>(i));
    double rsi = compute_rsi(prices);
    assert(rsi < 10.0 && "RSI pure down series < 10");
    std::cout << "  PASS  rsi_pure_down  (rsi=" << rsi << ")\n";
}

// ---------------------------------------------------------------------------
// Test: RSI on a flat series (no moves) should return 50
// ---------------------------------------------------------------------------
static void test_rsi_flat() {
    std::vector<Price> prices(20, Price(200.0));
    double rsi = compute_rsi(prices);
    assert(approx(rsi, 50.0) && "RSI flat series == 50");
    std::cout << "  PASS  rsi_flat\n";
}

// ---------------------------------------------------------------------------
// Test: Sentiment — bullish when price > SMA and RSI < 70
// ---------------------------------------------------------------------------
static void test_sentiment_bullish() {
    // Rising then flat: current price should be above SMA, RSI moderate
    std::vector<Price> prices;
    for (int i = 1; i <= 55; ++i) prices.emplace_back(static_cast<double>(i));
    // Add 15 flat days to cool RSI below 70
    for (int i = 0; i < 15; ++i) prices.emplace_back(55.0);

    Price  sma = compute_sma50(prices);
    double rsi = compute_rsi(prices);
    bool   above_sma = prices.back() > sma;
    std::string sentiment = (above_sma && rsi < 70.0) ? "Bullish"
                          : (!above_sma && rsi > 30.0) ? "Bearish"
                          : "Neutral";
    assert(sentiment == "Bullish" && "sentiment bullish");
    std::cout << "  PASS  sentiment_bullish  (RSI=" << rsi << " SMA=" << sma.to_double() << ")\n";
}

// ---------------------------------------------------------------------------
// Test: CircularBuffer evicts oldest entry correctly
// ---------------------------------------------------------------------------
static void test_circular_buffer_eviction() {
    CircularBuffer<Price, 3> buf;
    buf.push(Price(10.0));
    buf.push(Price(20.0));
    buf.push(Price(30.0));
    assert(approx(buf.avg(), 20.0) && "buf avg 10/20/30 == 20");
    buf.push(Price(40.0));  // evicts 10
    assert(approx(buf.avg(), 30.0) && "buf avg 20/30/40 == 30");
    std::cout << "  PASS  circular_buffer_eviction\n";
}

// ---------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------
int main() {
    std::cout << "=== analysis_engine unit tests ===\n\n";

    test_sma_flat();
    test_sma_rising();
    test_rsi_pure_up();
    test_rsi_pure_down();
    test_rsi_flat();
    test_sentiment_bullish();
    test_circular_buffer_eviction();

    std::cout << "\nAll tests passed.\n";
    return 0;
}
