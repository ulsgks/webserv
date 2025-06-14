#include "ServerBlock.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

// Server configuration constants
static const size_t DEFAULT_CLIENT_MAX_BODY_SIZE = 1024 * 1024;  // 1MB default
static const int DEFAULT_SERVER_PORT = 8080;                     // Default server port
static const int MIN_PORT_NUMBER = 1;                            // Minimum valid port
static const int MAX_PORT_NUMBER = 65535;                        // Maximum valid port

ServerBlock::ServerBlock()
    : client_max_body_size_set(false),
      client_max_body_size(DEFAULT_CLIENT_MAX_BODY_SIZE)  // 1MB default
      ,
      is_default(false) {
    // Add default listen directive
    listen.push_back(std::pair<std::string, int>("0.0.0.0", DEFAULT_SERVER_PORT));

    // Default root is empty
    root = "";
}

void ServerBlock::is_valid() const {
    validate_listen_directives();
    validate_locations();
    validate_root();
}

const LocationBlock* ServerBlock::match_location(const std::string& uri) const {
    // First, try to find an exact match
    for (LocationBlockVectorConstIt it = locations.begin(); it != locations.end(); ++it) {
        if (it->exact_match && uri == it->path) {
            return &(*it);
        }
    }

    // If no exact match, fall back to the longest matching prefix
    size_t longest_match = 0;
    const LocationBlock* matched_location = NULL;

    for (LocationBlockVectorConstIt it = locations.begin(); it != locations.end(); ++it) {
        // Skip exact match locations, as we already checked them
        if (it->exact_match) {
            continue;
        }

        // Check if this location's path is a prefix of the URI
        if (uri.find(it->path) == 0 && it->path.length() > longest_match) {
            longest_match = it->path.length();
            matched_location = &(*it);
        }
    }

    return matched_location;
}

bool ServerBlock::matches_server_name(const std::string& host) const {
    const std::string normalized_host = normalize_server_name(host);

    for (StringVectorConstIt it = server_names.begin(); it != server_names.end(); ++it) {
        if (normalize_server_name(*it) == normalized_host) {
            return true;
        }
    }
    return false;
}

void ServerBlock::validate_listen_directives() const {
    if (listen.empty()) {
        throw std::runtime_error("Server block is missing listen directives");
    }

    // Validate each listen directive
    for (ListenVectorConstIt it = listen.begin(); it != listen.end(); ++it) {
        // Check port range
        if (it->second < MIN_PORT_NUMBER || it->second > MAX_PORT_NUMBER) {
            std::stringstream ss;
            ss << "Invalid port number: " << it->second << " (must be between " << MIN_PORT_NUMBER
               << " and " << MAX_PORT_NUMBER << ")";
            throw std::runtime_error(ss.str());
        }

        // IP address validation could be added here
        // For now, basic validation to ensure IP is in a reasonable format
        const std::string& ip = it->first;
        if (!ip.empty() && ip != "0.0.0.0" && ip != "localhost" && ip != "*") {
            // Very basic IP format check - could be expanded
            bool valid_format = true;
            size_t dots = 0;

            for (size_t i = 0; i < ip.length(); ++i) {
                if (ip[i] == '.') {
                    dots++;
                } else if (!std::isdigit(ip[i])) {
                    valid_format = false;
                    break;
                }
            }

            if (!valid_format || dots != 3) {
                throw std::runtime_error("Invalid IP address format: " + ip);
            }
        }
    }
}

void ServerBlock::validate_locations() const {
    // Check each location block
    for (LocationBlockVectorConstIt it = locations.begin(); it != locations.end(); ++it) {
        try {
            it->is_valid();
        } catch (const std::exception& e) {
            std::string error_msg = "Invalid location block '" + it->path + "'";
            if (e.what() && *e.what()) {
                error_msg += ": ";
                error_msg += e.what();
            }
            throw std::runtime_error(error_msg);
        }
    }

    // Check for duplicate location paths
    for (size_t i = 0; i < locations.size(); ++i) {
        for (size_t j = i + 1; j < locations.size(); ++j) {
            // For exact matches, the paths must be different
            if (locations[i].exact_match && locations[j].exact_match &&
                locations[i].path == locations[j].path) {
                throw std::runtime_error("Duplicate exact match location: " + locations[i].path);
            }

            // For prefix matches with the same path, one must be exact
            if (!locations[i].exact_match && !locations[j].exact_match &&
                locations[i].path == locations[j].path) {
                throw std::runtime_error("Duplicate prefix location: " + locations[i].path);
            }
        }
    }
}

void ServerBlock::validate_root() const {
    // nginx do not require a server-level root directive, but can set a default
    // we choose to be strict and require it
    if (root.empty()) {
        throw std::runtime_error("Server block requires a root directive");
    }

    // Check for invalid characters in the path
    for (size_t i = 0; i < root.length(); ++i) {
        char c = root[i];
        // Allow standard path characters
        if (!std::isalnum(c) && c != '/' && c != '.' && c != '_' && c != '-') {
            throw std::runtime_error("Invalid character in server root path: " + std::string(1, c));
        }
    }
}

std::string ServerBlock::normalize_server_name(const std::string& name) const {
    std::string normalized;
    normalized.reserve(name.size());

    // Convert to lowercase and handle potential trailing dots
    for (StringConstIt it = name.begin(); it != name.end(); ++it) {
        normalized += std::tolower(*it);
    }

    // Remove trailing dots
    while (!normalized.empty() && normalized[normalized.size() - 1] == '.') {
        normalized.erase(normalized.size() - 1);
    }

    return normalized;
}
