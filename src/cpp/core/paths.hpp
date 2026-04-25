#pragma once

#include <string>
#include <filesystem>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace Axiom::Paths {

namespace fs = std::filesystem;

inline std::string get_axiom_dir() {
    const char* home = std::getenv("HOME");
    if (!home) return ".";
    return (fs::path(home) / ".axiom").string();
}

inline void ensure_axiom_dir() {
    fs::create_directories(get_axiom_dir());
}

inline std::string get_cache_dir() {
    return (fs::path(get_axiom_dir()) / "cache").string();
}

inline std::string get_path(const std::string& filename) {
    ensure_axiom_dir();
    fs::create_directories(get_cache_dir());
    return (fs::path(get_axiom_dir()) / filename).string();
}

inline std::string today_str() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

inline std::string date_years_ago(int years) {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    tm.tm_year -= years;
    std::mktime(&tm);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

} // namespace Axiom::Paths
