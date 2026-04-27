#pragma once
#include <string>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "core/paths.hpp"

namespace Axiom::Utils {

class Logger {
public:
    enum class Level { DEBUG, INFO, WARN, ERR };
    enum class Format { TEXT, JSON };

    struct Config {
        Level min_level = Level::INFO;
        Format format = Format::TEXT;
    };

    static Config& config() {
        static Config instance;
        return instance;
    }

    static void log(Level level, const std::string& message) {
        if (level < config().min_level) return;

        std::ofstream log_file(Paths::get_path("axiom.log"), std::ios::app);
        
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        
        std::ostringstream time_ss;
        time_ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

        if (config().format == Format::JSON) {
            std::string lvl_str;
            switch (level) {
                case Level::DEBUG: lvl_str = "DEBUG"; break;
                case Level::INFO:  lvl_str = "INFO";  break;
                case Level::WARN:  lvl_str = "WARN";  break;
                case Level::ERR:   lvl_str = "ERROR"; break;
            }
            std::string json = "{\"timestamp\":\"" + time_ss.str() + "\", \"level\":\"" + lvl_str + "\", \"message\":\"" + message + "\"}";
            if (log_file.is_open()) log_file << json << std::endl;
            if (level >= Level::INFO) std::cerr << json << std::endl;
        } else {
            std::ostringstream out;
            out << "[" << time_ss.str() << "] ";
            switch (level) {
                case Level::DEBUG: out << "DEBUG: "; break;
                case Level::INFO:  out << "INFO:  "; break;
                case Level::WARN:  out << "WARN:  "; break;
                case Level::ERR:   out << "ERROR: "; break;
            }
            out << message;
            if (log_file.is_open()) log_file << out.str() << std::endl;
            if (level >= Level::INFO) std::cerr << out.str() << std::endl;
        }
    }

    static void debug(const std::string& m) { log(Level::DEBUG, m); }
    static void info(const std::string& m)  { log(Level::INFO, m); }
    static void warn(const std::string& m)  { log(Level::WARN, m); }
    static void error(const std::string& m) { log(Level::ERR, m); }
};

} // namespace Axiom::Utils
