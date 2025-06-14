#include "LocationBlock.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

// Default configuration constants
static const size_t DEFAULT_CLIENT_MAX_BODY_SIZE = 1024 * 1024;  // 1MB default

LocationBlock::LocationBlock()
    : exact_match(false),
      autoindex(false),
      redirect_status_code(0),
      client_max_body_size(DEFAULT_CLIENT_MAX_BODY_SIZE),  // 1MB default
      client_max_body_size_set(false),
      cgi_enabled(false) {
    allowed_methods.push_back(HttpMethods::GET);  // Default GET
}

void LocationBlock::is_valid() const {
    validate_path();
    validate_methods();
    validate_root();
    validate_redirect_compatibilities();
    validate_cgi_configuration();
}

bool LocationBlock::is_allows_method(HttpMethods::Method method) const {
    // Search for method in allowed methods vector
    return std::find(allowed_methods.begin(), allowed_methods.end(), method) !=
           allowed_methods.end();
}

std::string LocationBlock::get_allowed_methods_string() const {
    std::string allowed_methods_string = "";
    bool first = true;

    // Only include methods that are both allowed AND implemented
    for (size_t i = 0; i < allowed_methods.size(); ++i) {
        if (HttpMethods::is_implemented(allowed_methods[i])) {
            if (!first) {
                allowed_methods_string += ", ";
            }
            allowed_methods_string += HttpMethods::to_string(allowed_methods[i]);
            first = false;
        }
    }

    // If no implemented methods are allowed, return at least GET
    if (allowed_methods_string.empty()) {
        allowed_methods_string = "GET";
    }

    return allowed_methods_string;
}

void LocationBlock::validate_path() const {
    if (path.empty()) {
        throw std::runtime_error("Location path cannot be empty");
    }

    if (path[0] != '/') {
        throw std::runtime_error("Location path must start with a slash (/)");
    }

    // Check for invalid characters in path
    for (size_t i = 0; i < path.length(); ++i) {
        char c = path[i];
        // Allow valid URL path characters
        if (!std::isalnum(c) && c != '/' && c != '.' && c != '_' && c != '-') {
            std::stringstream ss;
            ss << "Invalid character '" << c << "' in location path: " << path;
            throw std::runtime_error(ss.str());
        }
    }
}

void LocationBlock::validate_methods() const {
    if (allowed_methods.empty()) {
        throw std::runtime_error("Location must have at least one allowed HTTP method");
    }

    // Check for duplicate methods
    for (size_t i = 0; i < allowed_methods.size(); ++i) {
        for (size_t j = i + 1; j < allowed_methods.size(); ++j) {
            if (allowed_methods[i] == allowed_methods[j]) {
                std::string method_name = HttpMethods::to_string(allowed_methods[i]);
                throw std::runtime_error("Duplicate HTTP method in location: " + method_name);
            }
        }
    }
}

void LocationBlock::validate_root() const {
    // Root is optional in these cases:
    // 1. When there's a redirect directive
    // 2. For CGI locations
    // 3. When it can inherit from the server block (checked elsewhere)
    if (!redirect.empty() || cgi_enabled) {
        return;
    }

    // If root is provided, validate it
    if (!root.empty()) {
        // Check for invalid characters in the path
        for (size_t i = 0; i < root.length(); ++i) {
            char c = root[i];
            // Allow standard path characters
            if (!std::isalnum(c) && c != '/' && c != '.' && c != '_' && c != '-') {
                std::stringstream ss;
                ss << "Invalid character '" << c << "' in location root: " << root;
                throw std::runtime_error(ss.str());
            }
        }
    }
}

void LocationBlock::validate_redirect_compatibilities() const {
    // Check for incompatibilities with redirect directive
    if (!redirect.empty()) {
        if (!index.empty()) {
            throw std::runtime_error(
                "'return' and 'index' directives are incompatible in location block");
        }
        if (autoindex) {
            throw std::runtime_error(
                "'return' and 'autoindex' directives are incompatible in location block");
        }
        if (!upload_store.empty()) {
            throw std::runtime_error(
                "'return' and 'upload_store' directives are incompatible in location block");
        }
        if (!root.empty()) {
            throw std::runtime_error(
                "'return' and 'root' directives are incompatible in location block");
        }
    }
}

void LocationBlock::validate_cgi_configuration() const {
    // Check that CGI handlers aren't defined without CGI being enabled
    if (!cgi_handlers.empty() && !cgi_enabled) {
        throw std::runtime_error(
            "CGI handlers defined but CGI is not enabled with 'cgi_enabled on;'");
    }
}
