#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include <string_view>
#include <filesystem>

namespace Axiom::Service {

class ConfigService {
public:
    static bool load_config();
    static bool save_config();
    static std::string get_api_key();
    static void set_api_key(const std::string& key);

private:
    static std::unordered_map<std::string, std::string> m_config_map;
    static std::optional<std::string> resolve_api_key(std::string_view env_var_name, std::string_view config_key);
    static std::filesystem::path get_config_path();
};

} // namespace Axiom::Service
