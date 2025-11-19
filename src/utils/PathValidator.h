#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>

#include "Config.h"

namespace utils {

namespace fs = std::filesystem;

inline std::string validateUser(const std::string& user) {
    if (user.empty()) {
        throw std::invalid_argument("Missing user field");
    }

    // Path traversal prevention
    if (user.find("..") != std::string::npos ||
        user.find("/") != std::string::npos) {
        throw std::invalid_argument("Invalid user");
    }

    if (!fs::exists(Config::PATH_HOME_BASE + user)) {
        throw std::invalid_argument("User not found");
    }

    return user;
}

}  // namespace utils
