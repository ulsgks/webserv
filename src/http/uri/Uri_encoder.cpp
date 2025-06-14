#include <stdio.h>

#include <cctype>
#include <cstdlib>

#include "../error/Error.hpp"
#include "Uri.hpp"

// Converts special characters to %XX hex format
// Example: "a b" -> "a%20b"
std::string Uri::encode(const std::string& str) {
    std::string encoded;
    for (size_t i = 0; i < str.length(); ++i) {
        char c = str[i];
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            encoded += '%';
            char hex[3];
            snprintf(hex, sizeof(hex), "%02X", (unsigned char)c);
            encoded += hex;
        }
    }
    return encoded;
}

// Converts %XX hex sequences back to characters
// Example: "a%20b" -> "a b"
// (+ should remain as +, only percent-encoded characters are decoded)
std::string Uri::decode(const std::string& str) {
    std::string decoded;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            // Check for null byte
            if (str[i + 1] == '0' && str[i + 2] == '0') {
                throw HttpError(BAD_REQUEST, "Invalid URL: contains null byte");
            }

            std::string hex = str.substr(i + 1, 2);
            char c = (char)strtol(hex.c_str(), NULL, 16);
            decoded += c;
            i += 2;
        } else {
            decoded += str[i];
        }
    }
    return decoded;
}

// Decode specifically for query parameters
// (this handles both percent-encoding and + as space)
std::string Uri::decodeQueryParam(const std::string& str) {
    std::string decoded;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            // Check for null byte
            if (str[i + 1] == '0' && str[i + 2] == '0') {
                throw HttpError(BAD_REQUEST, "Invalid URL: contains null byte");
            }

            std::string hex = str.substr(i + 1, 2);
            char c = (char)strtol(hex.c_str(), NULL, 16);
            decoded += c;
            i += 2;
        } else if (str[i] == '+') {
            decoded += ' ';  // Convert + to space in query params
        } else {
            decoded += str[i];
        }
    }
    return decoded;
}
