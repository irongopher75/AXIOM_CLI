#include "data_engine.hpp"

#include <curl/curl.h>
#include <ctime>
#include <sstream>
#include "../json.hpp"
#include "../core/limiter.hpp"
#include "../core/error.hpp"
#include "../core/paths.hpp"

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// API keys — move to config/env in production
// ---------------------------------------------------------------------------
static const std::string POLYGON_API_KEY = "ZRWWaU0qlNwnx78exZY9UmMrTA_kpriU";
static const std::string FINNHUB_API_KEY = "d6lt9jhr01quej91dc10d6lt9jhr01quej91dc1g";

// ---------------------------------------------------------------------------
// Rate limiters — static so they persist for the process lifetime and the
// disk-backed vault files are opened exactly once.
// ---------------------------------------------------------------------------
static Axiom::TokenBucket poly_bucket(5,  5,  Axiom::Paths::get_path(".poly_vault"));
static Axiom::TokenBucket finn_bucket(60, 60, Axiom::Paths::get_path(".finn_vault"));

// ---------------------------------------------------------------------------
// Internal: libcurl write callback
// ---------------------------------------------------------------------------
static size_t write_cb(void* contents, size_t size, size_t nmemb, void* userp) {
    reinterpret_cast<std::string*>(userp)->append(
        reinterpret_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

// ---------------------------------------------------------------------------
// Internal: synchronous GET — returns empty string on any failure.
// TODO: replace with async epoll/kqueue loop for real-time use.
// ---------------------------------------------------------------------------
static std::string fetch_url(const std::string& url) {
    std::string body;
    CURL* curl = curl_easy_init();
    if (!curl) return body;

    curl_easy_setopt(curl, CURLOPT_URL,           url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &body);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,     "Mozilla/5.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,       10L);

    if (curl_easy_perform(curl) != CURLE_OK) body.clear();
    curl_easy_cleanup(curl);
    return body;
}

// ---------------------------------------------------------------------------
// Internal: date helpers
// ---------------------------------------------------------------------------
static std::string date_ago(int years) {
    std::time_t t = std::time(nullptr) - static_cast<std::time_t>(years) * 365 * 24 * 60 * 60;
    struct tm* tm = std::localtime(&t);
    char buf[11];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", tm);
    return buf;
}

static std::string today() {
    auto t = std::time(nullptr);
    struct tm* tm = std::localtime(&t);
    char buf[11];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", tm);
    return buf;
}

// ---------------------------------------------------------------------------
// Internal: parse helpers — return empty vector on any parse failure
// ---------------------------------------------------------------------------
static std::vector<Axiom::Price> parse_polygon(const std::string& raw) {
    std::vector<Axiom::Price> out;
    try {
        auto j = json::parse(raw);
        if (!j.contains("results")) return out;
        for (auto& r : j["results"])
            out.emplace_back(r["c"].get<double>());
    } catch (...) {}
    return out;
}

static std::vector<Axiom::Price> parse_finnhub(const std::string& raw) {
    std::vector<Axiom::Price> out;
    try {
        auto j = json::parse(raw);
        if (!j.contains("c")) return out;
        for (auto& c : j["c"])
            out.emplace_back(c.get<double>());
    } catch (...) {}
    return out;
}

static std::vector<Axiom::Price> parse_yahoo(const std::string& raw) {
    std::vector<Axiom::Price> out;
    try {
        auto j     = json::parse(raw);
        auto& bars = j["chart"]["result"][0]["indicators"]["quote"][0]["close"];
        for (auto& p : bars)
            if (!p.is_null()) out.emplace_back(p.get<double>());
    } catch (...) {}
    return out;
}

// ===========================================================================
// Public API
// ===========================================================================

namespace Axiom::DataEngine {

Result<PriceData> fetch_prices(const std::string& symbol, int years) {
    PriceData data;

    // Tier 1 — Polygon
    if (poly_bucket.consume()) {
        std::string url =
            "https://api.polygon.io/v2/aggs/ticker/" + symbol +
            "/range/1/day/" + date_ago(years) + "/" + today() +
            "?adjusted=true&sort=asc&apiKey=" + POLYGON_API_KEY;

        data.prices = parse_polygon(fetch_url(url));
        if (!data.prices.empty()) {
            data.source = "Polygon (Direct)";
            return Result<PriceData>(data);
        }
    }

    // Tier 2 — Finnhub
    if (finn_bucket.consume()) {
        long long to   = std::time(nullptr);
        long long from = to - static_cast<long long>(years) * 365 * 24 * 60 * 60;
        std::string url =
            "https://finnhub.io/api/v1/stock/candle?symbol=" + symbol +
            "&resolution=D&from=" + std::to_string(from) +
            "&to="   + std::to_string(to) +
            "&token=" + FINNHUB_API_KEY;

        data.prices = parse_finnhub(fetch_url(url));
        if (!data.prices.empty()) {
            data.source = "Finnhub (Institutional)";
            return Result<PriceData>(data);
        }
    }

    // Tier 3 — Yahoo (no rate limit key required, no token consumed)
    std::string url =
        "https://query1.finance.yahoo.com/v8/finance/chart/" + symbol +
        "?range=" + std::to_string(years) + "y&interval=1d";

    data.prices = parse_yahoo(fetch_url(url));
    if (!data.prices.empty()) {
        data.source = "Yahoo (Legacy Fallback)";
        return Result<PriceData>(data);
    }

    return Result<PriceData>(DataError::NetworkFailure);
}

std::vector<SymbolMatch> resolve_symbol(const std::string& query) {
    std::vector<SymbolMatch> matches;

    // Polygon search — consume a rate-limit token
    if (poly_bucket.consume()) {
        std::string url =
            "https://api.polygon.io/v3/reference/tickers?search=" + query +
            "&active=true&limit=5&apiKey=" + POLYGON_API_KEY;
        try {
            auto j = json::parse(fetch_url(url));
            for (auto& r : j["results"])
                matches.push_back({ r["ticker"], r["name"], r["primary_exchange"] });
            if (!matches.empty()) return matches;
        } catch (...) {}
    }

    // Finnhub search fallback
    if (finn_bucket.consume()) {
        std::string url =
            "https://finnhub.io/api/v1/search?q=" + query +
            "&token=" + FINNHUB_API_KEY;
        try {
            auto j = json::parse(fetch_url(url));
            for (auto& r : j["result"])
                matches.push_back({ r["symbol"], r["description"], "Global" });
        } catch (...) {}
    }

    return matches;
}

} // namespace Axiom::DataEngine
