// HttpHeaders.hpp
#ifndef HTTP_HEADERS_HPP
#define HTTP_HEADERS_HPP

#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <vector>

#include "../../utils/Types.hpp"

namespace HttpHeaders {
    typedef const char* HeaderName;

    // C++98 compatible helper function to create string pairs
    inline std::pair<std::string, std::string> make_pair(
        const std::string& first, const std::string& second) {
        std::pair<std::string, std::string> p;
        p.first = first;
        p.second = second;
        return p;
    }

    // Common HTTP header names
    // Request headers
    extern const HeaderName ACCEPT;
    extern const HeaderName ACCEPT_CHARSET;
    extern const HeaderName ACCEPT_ENCODING;
    extern const HeaderName ACCEPT_LANGUAGE;
    extern const HeaderName AUTHORIZATION;
    extern const HeaderName CONNECTION;
    extern const HeaderName CONTENT_LENGTH;
    extern const HeaderName CONTENT_TYPE;
    extern const HeaderName COOKIE;
    extern const HeaderName HOST;
    extern const HeaderName REFERER;
    extern const HeaderName USER_AGENT;
    extern const HeaderName TRANSFER_ENCODING;

    // Response headers
    extern const HeaderName ALLOW;
    extern const HeaderName CACHE_CONTROL;
    extern const HeaderName CONTENT_DISPOSITION;
    extern const HeaderName CONTENT_ENCODING;
    extern const HeaderName CONTENT_LANGUAGE;
    extern const HeaderName DATE;
    extern const HeaderName EXPIRES;
    extern const HeaderName LAST_MODIFIED;
    extern const HeaderName LOCATION;
    extern const HeaderName SERVER;
    extern const HeaderName SET_COOKIE;
    extern const HeaderName WWW_AUTHENTICATE;

    // Convert header name to lowercase for case-insensitive comparison
    inline std::string to_lowercase(const std::string& name) {
        std::string lowercase = name;
        std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(), ::tolower);
        return lowercase;
    }

    // Convert header name to the standard format (e.g., "content-type" -> "Content-Type")
    inline std::string normalize_name(const std::string& name) {
        std::string normalized = name;
        bool capitalize = true;

        for (size_t i = 0; i < normalized.length(); ++i) {
            if (capitalize) {
                normalized[i] = std::toupper(normalized[i]);
                capitalize = false;
            } else if (normalized[i] == '-') {
                capitalize = true;
            } else {
                normalized[i] = std::tolower(normalized[i]);
            }
        }

        return normalized;
    }

    // Case-insensitive string comparison for headers
    inline bool compare_insensitive(const std::string& a, const std::string& b) {
        if (a.length() != b.length()) {
            return false;
        }

        for (size_t i = 0; i < a.length(); ++i) {
            if (std::tolower(a[i]) != std::tolower(b[i])) {
                return false;
            }
        }

        return true;
    }

    // Check if a header value contains a specific token (case-insensitive)
    inline bool value_contains(const std::string& value, const std::string& token) {
        std::string lowercase_value = to_lowercase(value);
        std::string lowercase_token = to_lowercase(token);
        return lowercase_value.find(lowercase_token) != std::string::npos;
    }

    // Content-Length validation
    inline bool is_valid_content_length(const std::string& content_length) {
        // Content-Length must be a positive integer
        if (content_length.empty()) {
            return false;
        }

        for (size_t i = 0; i < content_length.length(); ++i) {
            if (!std::isdigit(content_length[i])) {
                return false;
            }
        }

        return true;
    }

    // ----- HeaderMap (multimap) versions of header functions -----

    // Check if a header exists in a header multimap (case-insensitive)
    inline bool has(const HeaderMap& headers, const std::string& name) {
        for (HeaderMapConstIt it = headers.begin(); it != headers.end(); ++it) {
            if (compare_insensitive(it->first, name)) {
                return true;
            }
        }

        return false;
    }

    // Get a header value from a header multimap (case-insensitive)
    // This returns the first matching header value
    inline std::string get(const HeaderMap& headers, const std::string& name) {
        for (HeaderMapConstIt it = headers.begin(); it != headers.end(); ++it) {
            if (compare_insensitive(it->first, name)) {
                return it->second;
            }
        }

        return "";
    }

    // Header classification functions for proper duplicate handling
    // according to RFC 7230 Section 3.2.2

    // Headers that MUST only appear once (single-value headers)
    inline bool is_single_value_header(const std::string& name) {
        std::string lower_name = to_lowercase(name);
        return (
            lower_name == "content-length" || lower_name == "content-type" ||
            lower_name == "date" || lower_name == "server" || lower_name == "location" ||
            lower_name == "last-modified" || lower_name == "expires" || lower_name == "etag" ||
            lower_name == "host" || lower_name == "authorization" || lower_name == "referer" ||
            lower_name == "user-agent");
    }

    // Headers that can appear multiple times but should NOT be combined with commas
    // These are special cases that need individual header lines
    inline bool is_special_multiple_header(const std::string& name) {
        std::string lower_name = to_lowercase(name);
        return (lower_name == "set-cookie" || lower_name == "www-authenticate");
    }

    // Headers that can appear multiple times and CAN be combined with commas
    inline bool is_combinable_header(const std::string& name) {
        std::string lower_name = to_lowercase(name);
        return (
            lower_name == "accept" || lower_name == "accept-charset" ||
            lower_name == "accept-encoding" || lower_name == "accept-language" ||
            lower_name == "cache-control" || lower_name == "content-encoding" ||
            lower_name == "content-language" || lower_name == "allow" || lower_name == "pragma" ||
            lower_name == "warning" ||
            // X-* headers are commonly used for custom headers that should be combinable
            (lower_name.length() > 2 && lower_name.substr(0, 2) == "x-"));
    }

    // Add header to multimap with RFC 7230 Section 3.2.2 compliance
    inline void add_header(HeaderMap& headers, const std::string& name, const std::string& value) {
        if (is_single_value_header(name)) {
            // Single-value headers: remove any existing value
            HeaderMapIt it = headers.find(name);
            if (it != headers.end()) {
                headers.erase(it);
            }
            headers.insert(make_pair(name, value));
        } else if (is_special_multiple_header(name)) {
            // Special multiple headers (like Set-Cookie): always store separately
            headers.insert(make_pair(name, value));
        } else if (is_combinable_header(name)) {
            // Combinable headers: check if we should combine values
            HeaderMapIt it = headers.find(name);
            if (it != headers.end()) {
                // Combine with existing value using comma separator
                std::string combined = it->second + ", " + value;
                headers.erase(it);
                headers.insert(make_pair(name, combined));
            } else {
                headers.insert(make_pair(name, value));
            }
        } else {
            // Unknown headers: treat as single-value for safety
            HeaderMapIt it = headers.find(name);
            if (it != headers.end()) {
                headers.erase(it);
            }
            headers.insert(make_pair(name, value));
        }
    }

}  // namespace HttpHeaders

// Convert HTTP header name to CGI environment variable format
// e.g., "User-Agent" -> "HTTP_USER_AGENT"
inline std::string to_cgi_env_name(const std::string& header_name) {
    std::string env_name = "HTTP_";
    for (size_t i = 0; i < header_name.size(); i++) {
        char c = header_name[i];
        if (c == '-') {
            env_name += '_';
        } else {
            env_name += std::toupper(c);
        }
    }
    return env_name;
}

#endif  // HTTP_HEADERS_HPP
