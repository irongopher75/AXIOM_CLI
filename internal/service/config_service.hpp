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
};

} // namespace Axiom::Service
