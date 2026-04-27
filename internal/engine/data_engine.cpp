#include "data_engine.hpp"
#include <iostream>
#include <algorithm>
#include <curl/curl.h>
#include "json/json.hpp"
#include <fstream>
#include <filesystem>
#include "core/paths.hpp"
#include "service/config_service.hpp"
#include "utils/logger.hpp"

using json = nlohmann::json;
using namespace Axiom::Service;
using namespace Axiom::Utils;

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

    std::string api_key = ConfigService::get_api_key();
    if (api_key.empty()) api_key = "V6S0E1AUPN_X9X6_7Z0Z8_Y1_2_3_4";

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
                        item.value("primary_exchange", "OTC"),
                        item.value("locale", "us")
                    });
                }
            } else if (data.contains("results") == false && data.contains("ticker")) {
                // Handle exact ticker response if applicable
                matches.push_back({
                    data.value("ticker", ""),
                    data.value("name", ""),
                    data.value("primary_exchange", "OTC"),
                    data.value("locale", "us")
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
                matches.push_back({ 
                    res.value("ticker", ""), 
                    res.value("name", ""), 
                    res.value("primary_exchange", "OTC"),
                    res.value("locale", "US") 
                });
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

Expected<PriceData, DataError> fetch_prices(std::string symbol, int years) {
    std::transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);
    if (auto cached = try_load_cache(symbol)) return Expected<PriceData, DataError>(*cached);

    std::string api_key = ConfigService::get_api_key();
    if (api_key.empty()) {
        Logger::error("API Key is missing. Cannot fetch prices for " + symbol);
        return DataError::AuthError;
    }

    auto end_date = Axiom::Paths::today_str();
    auto start_date = Axiom::Paths::date_years_ago(years);

    std::string url = "https://api.polygon.io/v2/aggs/ticker/" + symbol + 
                      "/range/1/day/" + start_date + "/" + end_date + 
                      "?adjusted=true&sort=asc&apiKey=" + api_key;
    
    Logger::info("Fetching prices for " + symbol + " from " + start_date + " to " + end_date);
    auto raw = fetch_raw_url(url);
    if (raw.empty()) {
        Logger::error("Network failure fetching prices for " + symbol + ". URL: " + url);
        return DataError::NetworkFailure;
    }

    try {
        auto data = json::parse(raw);
        auto status = data.value("status", "");
        if (status == "ERROR" && data.contains("error")) {
            std::string err_msg = data["error"];
            if (err_msg.find("API key") != std::string::npos) return DataError::AuthError;
        }

        if ((status != "OK" && status != "DELAYED") || !data.contains("results")) {
            Logger::warn("API returned non-OK status for " + symbol + ": " + status);
            return DataError::TickerNotFound;
        }

        PriceData pd;
        pd.source = (status == "DELAYED" ? "Polygon (Delayed)" : "Polygon.io");
        for (auto& bar : data["results"]) {
            pd.bars.push_back({
                Price(bar.value("o", 0.0)),
                Price(bar.value("h", 0.0)),
                Price(bar.value("l", 0.0)),
                Price(bar.value("c", 0.0)),
                bar.value("v", (int64_t)0)
            });
        }
        
        // Fetch Metadata (Exchange/Country)
        std::string meta_url = "https://api.polygon.io/v3/reference/tickers/" + symbol + "?apiKey=" + api_key;
        auto meta_raw = fetch_raw_url(meta_url);
        if (!meta_raw.empty()) {
            try {
                auto meta = json::parse(meta_raw);
                if (meta.contains("results")) {
                    pd.exchange = meta["results"].value("primary_exchange", "N/A");
                    pd.country  = meta["results"].value("locale", "us");
                    std::transform(pd.country.begin(), pd.country.end(), pd.country.begin(), ::toupper);
                }
            } catch (...) {}
        }

        save_cache(symbol, pd);
        return Expected<PriceData, DataError>(pd);
    } catch (const std::exception& e) {
        Logger::error("JSON parse error: " + std::string(e.what()));
        return DataError::ParseError;
    } catch (...) {
        return DataError::InternalError;
    }
}

} // namespace Axiom::DataEngine
