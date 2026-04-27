#include "config_service.hpp"
#include "core/paths.hpp"
#include "json/json.hpp"
#include <fstream>
#include <filesystem>

using json = nlohmann::json;

#include <iostream>
#include <array>
#include <memory>
#include <cstdio>

namespace Axiom::Service {

std::string ConfigService::api_key = "";

std::string ConfigService::get_machine_uuid() {
#ifdef __APPLE__
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("ioreg -rd1 -c IOPlatformExpertDevice | grep -E 'IOPlatformUUID' | awk -F' = ' '{print $2}' | tr -d '\"'", "r"), pclose);
    if (!pipe) return "DEFAULT_SALT_AXIOM";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    result.erase(result.find_last_not_of(" \n\r\t") + 1);
    return result.empty() ? "DEFAULT_SALT_AXIOM" : result;
#else
    return "LINUX_SALT_PLACEHOLDER";
#endif
}

std::string ConfigService::encrypt(const std::string& plaintext) {
    if (plaintext.empty()) return "";
    std::string uuid = get_machine_uuid();
    std::string ciphertext = plaintext;
    for (size_t i = 0; i < ciphertext.size(); ++i) {
        ciphertext[i] ^= uuid[i % uuid.size()];
    }
    // Simple hex encoding for visibility in JSON
    static const char* hex = "0123456789ABCDEF";
    std::string encoded;
    for (unsigned char c : ciphertext) {
        encoded += hex[c >> 4];
        encoded += hex[c & 0xf];
    }
    return "ENC:" + encoded;
}

std::string ConfigService::decrypt(const std::string& ciphertext) {
    if (ciphertext.substr(0, 4) != "ENC:") return ciphertext;
    std::string raw;
    std::string hex_part = ciphertext.substr(4);
    for (size_t i = 0; i < hex_part.size(); i += 2) {
        std::string byteString = hex_part.substr(i, 2);
        char byte = (char) strtol(byteString.c_str(), NULL, 16);
        raw += byte;
    }
    std::string uuid = get_machine_uuid();
    for (size_t i = 0; i < raw.size(); ++i) {
        raw[i] ^= uuid[i % uuid.size()];
    }
    return raw;
}

bool ConfigService::load_config() {
    auto path = Paths::get_path("config.json");
    if (!std::filesystem::exists(path)) return false;

    try {
        std::ifstream f(path);
        json j; f >> j;
        
        // Schema Validation
        if (!j.contains("api_key") || !j["api_key"].is_string()) {
            std::cerr << "✖ Config Error: Missing or invalid 'api_key' field.\n";
            return false;
        }

        api_key = decrypt(j["api_key"].get<std::string>());
        return true;
    } catch (const std::exception& e) {
        std::cerr << "✖ Config Parse Error: " << e.what() << "\n";
        return false;
    }
}

bool ConfigService::save_config() {
    auto path = Paths::get_path("config.json");
    try {
        json j;
        j["api_key"] = encrypt(api_key);
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
