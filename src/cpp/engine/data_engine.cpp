#include "data_engine.hpp"
#include <iostream>
#include <algorithm>
#include <curl/curl.h>
#include "../json.hpp"
#include <fstream>
#include <filesystem>
#include "core/paths.hpp"

using json = nlohmann::json;

namespace Axiom::DataEngine {

// ---------------------------------------------------------------------------
// CURL Plumbing
// ---------------------------------------------------------------------------

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

static std::string fetch_raw_url(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string buffer;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    return (res == CURLE_OK) ? buffer : "";
}

// ---------------------------------------------------------------------------
// Internal: Search Ranking
// ---------------------------------------------------------------------------

static int score_match(const std::string& query, const SymbolMatch& match) {
    std::string q = query;
    std::transform(q.begin(), q.end(), q.begin(), ::toupper);
    
    std::string s = match.symbol;
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    
    std::string n = match.name;
    std::transform(n.begin(), n.end(), n.begin(), ::toupper);

    if (s == q) return 1000;          
    if (s.find(q) == 0) return 800;   
    if (n.find(q) == 0) return 500;   
    if (s.find(q) != std::string::npos) return 300; 
    if (n.find(q) != std::string::npos) return 100; 
    
    return 0;
}

// ---------------------------------------------------------------------------
// Polygon Search Engine
// ---------------------------------------------------------------------------

std::vector<SymbolMatch> resolve_symbol(const std::string& query) {
    if (query.empty()) return {};

    const char* env_key = std::getenv("POLYGON_API_KEY");
    std::string api_key = env_key ? env_key : "V6S0E1AUPN_X9X6_7Z0Z8_Y1_2_3_4";

    // Try fuzzy search first
    std::string url = "https://api.polygon.io/v3/reference/tickers?search=" + query + 
                      "&active=true&limit=10&apiKey=" + api_key;
    
    auto raw = fetch_raw_url(url);
    std::vector<SymbolMatch> matches;

    auto parse_results = [&](const std::string& r) {
        try {
            auto data = json::parse(r);
            if (data.contains("results")) {
                for (auto& item : data["results"]) {
                    matches.push_back({
                        item.value("ticker", ""),
                        item.value("name", ""),
                        item.value("primary_exchange", "OTC")
                    });
                }
            } else if (data.contains("results") == false && data.contains("ticker")) {
                // Handle exact ticker response if applicable
                matches.push_back({
                    data.value("ticker", ""),
                    data.value("name", ""),
                    data.value("primary_exchange", "OTC")
                });
            }
        } catch (...) {}
    };

    parse_results(raw);

    // If no good matches, try exact ticker search
    if (matches.empty() && query.find(' ') == std::string::npos) {
        std::string exact_url = "https://api.polygon.io/v3/reference/tickers/" + query + 
                                "?apiKey=" + api_key;
        auto ex_raw = fetch_raw_url(exact_url);
        try {
            auto data = json::parse(ex_raw);
            if (data.contains("results")) {
                auto res = data["results"];
                matches.push_back({ res.value("ticker", ""), res.value("name", ""), res.value("primary_exchange", "OTC") });
            }
        } catch (...) {}
    }

    if (matches.empty()) return {};

    std::sort(matches.begin(), matches.end(), [&](const SymbolMatch& a, const SymbolMatch& b) {
        return score_match(query, a) > score_match(query, b);
    });

    // Remove duplicates
    matches.erase(std::unique(matches.begin(), matches.end(), [](const SymbolMatch& a, const SymbolMatch& b) {
        return a.symbol == b.symbol;
    }), matches.end());

    if (matches.size() > 5) matches.resize(5);
    return matches;
}

// ---------------------------------------------------------------------------
// Disk Cache Layer
// ---------------------------------------------------------------------------

static std::string get_cache_path(const std::string& symbol) {
    return Axiom::Paths::get_path("cache/" + symbol + ".json");
}

static std::optional<PriceData> try_load_cache(const std::string& symbol) {
    auto path = get_cache_path(symbol);
    if (!std::filesystem::exists(path)) return std::nullopt;

    try {
        std::ifstream f(path);
        json j; f >> j;
        
        auto today = Axiom::Paths::today_str();
        if (j.value("fetched_at", "") != today) return std::nullopt;

        PriceData pd;
        pd.source = "Local Cache";
        for (auto& b : j["bars"]) {
            pd.bars.push_back({
                Price(b[0].get<double>()),
                Price(b[1].get<double>()),
                Price(b[2].get<double>()),
                Price(b[3].get<double>()),
                b[4].get<int64_t>()
            });
        }
        return pd;
    } catch (...) { return std::nullopt; }
}

static void save_cache(const std::string& symbol, const PriceData& data) {
    try {
        json j;
        j["fetched_at"] = Axiom::Paths::today_str();
        j["bars"] = json::array();
        for (auto& b : data.bars) {
            j["bars"].push_back({
                b.o.to_double(), b.h.to_double(), b.l.to_double(), b.c.to_double(), b.v
            });
        }
        std::ofstream f(get_cache_path(symbol));
        f << j.dump(2);
    } catch (...) {}
}

// ---------------------------------------------------------------------------
// Core Data Fetcher
// ---------------------------------------------------------------------------

Result<PriceData> fetch_prices(const std::string& symbol, int years) {
    if (auto cached = try_load_cache(symbol)) return Result<PriceData>::success(*cached);

    const char* env_key = std::getenv("POLYGON_API_KEY");
    std::string api_key = env_key ? env_key : "V6S0E1AUPN_X9X6_7Z0Z8_Y1_2_3_4";

    auto end_date = Axiom::Paths::today_str();
    auto start_date = Axiom::Paths::date_years_ago(years);

    std::string url = "https://api.polygon.io/v2/aggs/ticker/" + symbol + 
                      "/range/1/day/" + start_date + "/" + end_date + 
                      "?adjusted=true&sort=asc&apiKey=" + api_key;
    
    auto raw = fetch_raw_url(url);
    if (raw.empty()) return Result<PriceData>::fail(DataError::NetworkFailure);

    try {
        auto data = json::parse(raw);
        if (data.value("status", "") != "OK" || !data.contains("results")) 
            return Result<PriceData>::fail(DataError::TickerNotFound);

        PriceData pd;
        pd.source = "Polygon.io";
        for (auto& bar : data["results"]) {
            pd.bars.push_back({
                Price(bar.value("o", 0.0)),
                Price(bar.value("h", 0.0)),
                Price(bar.value("l", 0.0)),
                Price(bar.value("c", 0.0)),
                bar.value("v", (int64_t)0)
            });
        }
        save_cache(symbol, pd);
        return Result<PriceData>::success(pd);
    } catch (...) {
        return Result<PriceData>::fail(DataError::NetworkFailure);
    }
}

} // namespace Axiom::DataEngine
