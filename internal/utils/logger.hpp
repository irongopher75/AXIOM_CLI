#pragma once

#include <string>
#include <fstream>
#include <ctime>
#include <iomanip>
#include "core/paths.hpp"

namespace Axiom::Utils {

class Logger {
public:
    enum class Level { INFO, WARN, ERR };

    static void log(Level level, const std::string& message) {
        std::ofstream log_file(Paths::get_path("axiom.log"), std::ios::app);
        if (!log_file.is_open()) return;

        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        
        log_file << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] ";
        
        switch (level) {
            case Level::INFO: log_file << "INFO: "; break;
            case Level::WARN: log_file << "WARN: "; break;
            case Level::ERR:  log_file << "ERROR: "; break;
        }
        
        log_file << message << std::endl;
    }

    static void info(const std::string& m) { log(Level::INFO, m); }
    static void warn(const std::string& m) { log(Level::WARN, m); }
    static void error(const std::string& m) { log(Level::ERR, m); }
};

} // namespace Axiom::Utils
