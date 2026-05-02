#include "config_service.hpp"
#include "core/paths.hpp"
#include "utils/logger.hpp"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>

namespace Axiom::Service {

std::unordered_map<std::string, std::string> ConfigService::m_config_map;

std::filesystem::path ConfigService::get_config_path() {
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME")) {
        if (xdg[0] != '\0') {
            return std::filesystem::path(xdg) / "axiom" / "config";
        }
    }

    const char* home = std::getenv("HOME");
    if (!home || home[0] == '\0') {
        home = getpwuid(getuid())->pw_dir;
    }

    return std::filesystem::path(home) / ".config" / "axiom" / "config";
}

std::optional<std::string> ConfigService::resolve_api_key(std::string_view env_var_name, std::string_view config_key) {
    if (const char* env_val = std::getenv(env_var_name.data())) {
        if (env_val[0] != '\0') {
            return std::string(env_val);
        }
    }

    auto it = m_config_map.find(std::string(config_key));
    if (it != m_config_map.end() && !it->second.empty()) {
        return it->second;
    }

    return std::nullopt;
}

bool ConfigService::load_config() {
    auto path = get_config_path();
    if (!std::filesystem::exists(path)) return false;

    try {
        std::ifstream file(path);
        if (!file.is_open()) return false;

        std::string line;
        while (std::getline(file, line)) {
            auto pos = line.find('=');
            if (pos != std::string::npos) {
                std::string k = line.substr(0, pos);
                std::string v = line.substr(pos + 1);
                m_config_map[k] = v;
            }
        }
        return true;
    } catch (const std::exception& e) {
        Utils::Logger::error("Config Parse Error: " + std::string(e.what()));
        return false;
    }
}

bool ConfigService::save_config() {
    auto path = get_config_path();
    try {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec) {
            Utils::Logger::error("Failed to create config directory: " + ec.message());
            return false;
        }

        std::ofstream file(path);
        if (!file.is_open()) {
            Utils::Logger::error("Failed to open config file for writing: " + path.string());
            return false;
        }

        for (const auto& [key, value] : m_config_map) {
            file << key << "=" << value << "\n";
        }
        file.close();

        // Set permissions to 600 — owner read/write only
        if (chmod(path.c_str(), S_IRUSR | S_IWUSR) != 0) {
            Utils::Logger::warn("Warning: could not set config file permissions to 600");
        }
        return true;
    } catch (const std::exception& e) {
        Utils::Logger::error("Config Save Error: " + std::string(e.what()));
        return false;
    }
}

std::string ConfigService::get_api_key() {
    auto key = resolve_api_key("AXIOM_API_KEY_POLYGON", "POLYGON_API_KEY");
    if (key) return *key;
    return "";
}

void ConfigService::set_api_key(const std::string& key) {
    m_config_map["POLYGON_API_KEY"] = key;
    save_config();
}

} // namespace Axiom::Service
