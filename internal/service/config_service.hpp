#pragma once

#include <string>
#include <map>

namespace Axiom::Service {

class ConfigService {
public:
    static bool load_config();
    static bool save_config();
    static std::string get_api_key();
    static void set_api_key(const std::string& key);
    
private:
    static std::string api_key;
    static std::string encrypt(const std::string& plaintext);
    static std::string decrypt(const std::string& ciphertext);
    static std::string get_machine_uuid();
};

} // namespace Axiom::Service
