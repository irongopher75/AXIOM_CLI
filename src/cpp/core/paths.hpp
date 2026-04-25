#pragma once

#include <string>
#include <filesystem>
#include <cstdlib>

namespace Axiom::Paths {

namespace fs = std::filesystem;

/**
 * @brief Returns the base directory for Axiom persistent data (~/.axiom).
 */
inline std::string get_axiom_dir() {
    const char* home = std::getenv("HOME");
    if (!home) return "."; // Fallback to CWD if HOME is missing
    return (fs::path(home) / ".axiom").string();
}

/**
 * @brief Ensures the ~/.axiom directory exists.
 */
inline void ensure_axiom_dir() {
    fs::create_directories(get_axiom_dir());
}

/**
 * @brief Returns the full path for a specific file inside ~/.axiom.
 */
inline std::string get_path(const std::string& filename) {
    ensure_axiom_dir();
    return (fs::path(get_axiom_dir()) / filename).string();
}

} // namespace Axiom::Paths
