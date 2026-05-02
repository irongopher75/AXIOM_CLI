#include "data_engine.hpp"
#include <iostream>
#include <algorithm>
#include <set>
#include <curl/curl.h>
#include "json/json.hpp"
#include <fstream>
#include <filesystem>
#include "core/paths.hpp"
#include "service/config_service.hpp"
#include "utils/logger.hpp"
#include "data_store.hpp"

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
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (compatible; Axiom/1.0)");
    
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
    if (api_key.empty()) {
        Logger::error("API Key is missing. Cannot resolve symbol: " + query);
        return {};
    }

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

    // Remove duplicates while preserving score order
    std::vector<SymbolMatch> unique_matches;
    std::set<std::string> seen;
    for (const auto& m : matches) {
        if (seen.find(m.symbol) == seen.end()) {
            unique_matches.push_back(m);
            seen.insert(m.symbol);
        }
    }
    matches = std::move(unique_matches);

    if (matches.size() > 5) matches.resize(5);
    return matches;
}

// ---------------------------------------------------------------------------
// Disk Cache Layer
// ---------------------------------------------------------------------------
// DuckDB caching is now handled by duckdb_cache.hpp/cpp

// ---------------------------------------------------------------------------
// Core Data Fetcher
// ---------------------------------------------------------------------------

Expected<PriceData, AxiomError> fetch_prices(std::string symbol, int years) {
    std::transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);
    
    auto today = Axiom::Paths::today_str();
    if (auto store = get_store()) {
        auto cached_bars = store->query_bars(symbol, today);
        if (!cached_bars.empty()) {
            PriceData pd;
            pd.source = "Local Cache (SQLite)";
            pd.bars = std::move(cached_bars);
            // Optional: You could fetch country/exchange metadata from another cache table later.
            return Expected<PriceData, AxiomError>(pd);
        }
    }

    std::string api_key = ConfigService::get_api_key();
    if (api_key.empty()) {
        Logger::error("API Key is missing. Cannot fetch prices for " + symbol);
        return AxiomError::AuthFailed;
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
        return AxiomError::NetworkTimeout;
    }

    try {
        auto data = json::parse(raw);
        auto status = data.value("status", "");
        if (status == "ERROR" && data.contains("error")) {
            std::string err_msg = data["error"];
            if (err_msg.find("API key") != std::string::npos) return AxiomError::AuthFailed;
        }

        if ((status != "OK" && status != "DELAYED") || !data.contains("results")) {
            // Fallback to Yahoo Finance
            std::string yf_url = "https://query1.finance.yahoo.com/v8/finance/chart/" + symbol + "?range=" + std::to_string(years) + "y&interval=1d";
            auto yf_raw = fetch_raw_url(yf_url);

            try {
                auto yf_data = json::parse(yf_raw);
                if (yf_data.contains("chart") &&
                    yf_data["chart"].contains("result") &&
                    !yf_data["chart"]["result"].is_null() &&
                    !yf_data["chart"]["result"].empty()) {

                    auto result = yf_data["chart"]["result"][0];

                    // Validate required fields exist before accessing
                    if (!result.contains("indicators") ||
                        !result["indicators"].contains("quote") ||
                        result["indicators"]["quote"].is_null() ||
                        result["indicators"]["quote"].empty()) {
                        Logger::warn("Yahoo Finance returned incomplete data for " + symbol);
                    } else {
                        auto quote = result["indicators"]["quote"][0];

                        // Validate quote subfields
                        if (quote.contains("open") && quote.contains("close") &&
                            !quote["open"].is_null() && !quote["close"].is_null()) {

                            auto timestamps = result["timestamp"];
                            auto opens = quote["open"];
                            auto highs = quote["high"];
                            auto lows = quote["low"];
                            auto closes = quote["close"];
                            auto volumes = quote["volume"];

                            PriceData pd;
                            pd.source = "Yahoo Finance";
                            pd.exchange = result["meta"].value("exchangeName", "N/A");
                            pd.country = "GLOBAL";

                            size_t count = std::min({opens.size(), highs.size(), lows.size(),
                                                     closes.size(), volumes.size(), timestamps.size()});
                            for (size_t i = 0; i < count; ++i) {
                                if (opens[i].is_null() || closes[i].is_null()) continue;
                                pd.bars.push_back({
                                    Price(opens[i].get<double>()),
                                    Price(highs[i].get<double>()),
                                    Price(lows[i].get<double>()),
                                    Price(closes[i].get<double>()),
                                    volumes[i].is_null() ? (int64_t)0 : volumes[i].get<int64_t>()
                                });
                            }
                            if (!pd.bars.empty()) {
                                if (auto store = get_store()) store->write_bars(symbol, pd.bars, pd.source, today);
                                return Expected<PriceData, AxiomError>(pd);
                            }
                        } else {
                            Logger::warn("Yahoo Finance quote data missing for " + symbol);
                        }
                    }
                } else {
                    Logger::warn("Yahoo Finance chart result empty for " + symbol);
                }
            } catch (const std::exception& e) {
                Logger::error("Yahoo Finance parse error: " + std::string(e.what()));
            } catch (...) {
                Logger::error("Yahoo Finance unknown error for " + symbol);
            }

            Logger::warn("API returned non-OK status for " + symbol + ": " + status);
            return AxiomError::TickerNotFound;
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

        if (auto store = get_store()) store->write_bars(symbol, pd.bars, pd.source, today);
        return Expected<PriceData, AxiomError>(pd);
    } catch (const std::exception& e) {
        Logger::error("JSON parse error: " + std::string(e.what()));
        return AxiomError::ParseError;
    } catch (...) {
        return AxiomError::InternalError;
    }
}

} // namespace Axiom::DataEngine
