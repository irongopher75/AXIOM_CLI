#pragma once
#include <ctime>
#include <string>
#include <fstream>
#include <algorithm>

namespace Axiom {

class TokenBucket {
private:
    double capacity;
    double tokens;
    double refill_rate; // tokens per second
    std::time_t last_refill;
    std::string persistence_path;

public:
    TokenBucket(double cap, double refill_per_min, const std::string& path) 
        : capacity(cap), tokens(cap), refill_rate(refill_per_min / 60.0), 
          last_refill(std::time(nullptr)), persistence_path(path) {
        load();
    }

    bool consume(double amount = 1.0) {
        refill();
        if (tokens >= amount) {
            tokens -= amount;
            save();
            return true;
        }
        return false;
    }

    void refill() {
        std::time_t now = std::time(nullptr);
        double delta = std::difftime(now, last_refill);
        tokens = std::min(capacity, tokens + delta * refill_rate);
        last_refill = now;
    }

    void save() {
        std::ofstream ofs(persistence_path);
        if (ofs.is_open()) {
            ofs << tokens << " " << last_refill;
        }
    }

    void load() {
        std::ifstream ifs(persistence_path);
        if (ifs.is_open()) {
            ifs >> tokens >> last_refill;
            refill(); // catch up since last close
        }
    }
};

} // namespace Axiom
