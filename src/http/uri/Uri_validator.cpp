#include <cctype>

#include "../../utils/Constants.hpp"
#include "../error/Error.hpp"
#include "Uri.hpp"

// URI validation methods
bool Uri::validate_uri(const std::string& uri) {
    // Check all characters and encoding
    if (!validate_uri_characters(uri)) {
        return false;
    }

    // If URI has a scheme, validate it
    if (uri.find(":// ") != std::string::npos) {
        if (!validate_uri_scheme(uri)) {
            return false;
        }
    }

    return true;
}

bool Uri::validate_uri_size(const std::string& uri) {
    // Check for empty URI
    if (uri.empty()) {
        return false;
    }

    // Check URI length
    if (uri.length() > Constants::Limits::MAX_URI_LENGTH) {
        return false;
    }

    return true;
}

bool Uri::validate_uri_characters(const std::string& uri) {
    // ASCII printable character range constants
    static const char ASCII_PRINTABLE_MIN = 32;   // Space character
    static const char ASCII_PRINTABLE_MAX = 126;  // Tilde character

    for (size_t i = 0; i < uri.length(); ++i) {
        char c = uri[i];

        // Control characters and non-ASCII are invalid
        if (c < ASCII_PRINTABLE_MIN || c > ASCII_PRINTABLE_MAX) {
            return false;
        }

        // Check percent encoding
        if (c == '%') {
            if (!validate_percent_encoding(uri, i)) {
                return false;
            }
            // Skip the two hex digits that were checked in validate_percent_encoding
            i += 2;
        }

        // Space and other special characters should be percent-encoded
        if (c == ' ' || c == '<' || c == '>' || c == '"' || c == '{' || c == '}' || c == '|' ||
            c == '\\' || c == '^' || c == '[' || c == ']' || c == '`') {
            return false;
        }
    }

    return true;
}

bool Uri::validate_percent_encoding(const std::string& uri, size_t index) {
    // Need at least 2 more chars for valid percent encoding
    if (index + 2 >= uri.length()) {
        return false;
    }

    // Percent encodings should be followed by two hex digits
    if (!std::isxdigit(uri[index + 1]) || !std::isxdigit(uri[index + 2])) {
        return false;
    }

    // Check for encoded null bytes (%00)
    if (uri[index + 1] == '0' && uri[index + 2] == '0') {
        return false;  // Reject URLs with encoded null bytes
    }

    return true;
}

bool Uri::validate_uri_scheme(const std::string& uri) {
    size_t scheme_end = uri.find(":// ");
    std::string scheme = uri.substr(0, scheme_end);

    // Scheme must start with a letter
    if (scheme.empty() || !std::isalpha(scheme[0])) {
        return false;
    }

    // Scheme can only contain letters, digits, plus, period, or hyphen
    for (size_t i = 1; i < scheme.length(); ++i) {
        char c = scheme[i];
        if (!std::isalnum(c) && c != '+' && c != '.' && c != '-') {
            return false;
        }
    }

    return true;
}

// Instance validation method
void Uri::validate() {
    valid_ = true;

    // Check for empty path
    if (path_.empty()) {
        path_ = "/";
    }

    // Check if path starts with '/'
    if (path_[0] != '/') {
        valid_ = false;
        return;
    }

    // Check URI length
    std::string full_uri = to_string();
    if (full_uri.length() > MAX_URI_LENGTH) {
        valid_ = false;
        return;
    }

    // Check for invalid characters in path
    static const char ASCII_PRINTABLE_MIN = 32;   // Space character
    static const char ASCII_PRINTABLE_MAX = 126;  // Tilde character

    for (size_t i = 0; i < path_.length(); ++i) {
        char c = path_[i];
        if (c < ASCII_PRINTABLE_MIN || c > ASCII_PRINTABLE_MAX) {
            valid_ = false;
            return;
        }
    }
}
