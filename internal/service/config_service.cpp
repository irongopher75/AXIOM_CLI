#include "config_service.hpp"
#include "core/paths.hpp"
#include "json/json.hpp"
#include <fstream>
#include <filesystem>

using json = nlohmann::json;

namespace Axiom::Service {

std::string ConfigService::api_key = "";

bool ConfigService::load_config() {
    auto path = Paths::get_path("config.json");
    if (!std::filesystem::exists(path)) return false;

    try {
        std::ifstream f(path);
        json j; f >> j;
        api_key = j.value("api_key", "");
        return true;
    } catch (...) {
        return false;
    }
}

bool ConfigService::save_config() {
    auto path = Paths::get_path("config.json");
    try {
        json j;
        j["api_key"] = api_key;
        std::ofstream f(path);
        f << j.dump(4);
        return true;
    } catch (...) {
        return false;
    }
}

std::string ConfigService::get_api_key() {
    if (api_key.empty()) {
        const char* env = std::getenv("POLYGON_API_KEY");
        if (env) return std::string(env);
    }
    return api_key;
}

void ConfigService::set_api_key(const std::string& key) {
    api_key = key;
    save_config();
}

} // namespace Axiom::Service
