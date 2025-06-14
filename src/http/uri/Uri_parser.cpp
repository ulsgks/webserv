#include <algorithm>
#include <sstream>
#include <vector>

#include "../../utils/Types.hpp"
#include "../error/Error.hpp"
#include "Uri.hpp"

// Constructors
Uri::Uri() : port_(HTTP_DEFAULT_PORT), absolute_(false), valid_(true) {
}

Uri::Uri(const std::string& uri_string) : port_(HTTP_DEFAULT_PORT), absolute_(false), valid_(true) {
    parse(uri_string);
}

// Main parsing method
void Uri::parse(const std::string& uri_string) {
    // Check if this is an absolute URI
    if (is_absolute_uri(uri_string)) {
        parse_absolute_uri(uri_string);
    } else {
        // Handle relative URI
        path_ = uri_string;
        extract_query_string(uri_string);
        normalize_path();
    }

    parse_query_params();
    validate();
}

// Check if URI is absolute
bool Uri::is_absolute_uri(const std::string& uri_string) {
    size_t scheme_end = uri_string.find("://");

    if (scheme_end != std::string::npos && scheme_end > 0) {
        std::string scheme = uri_string.substr(0, scheme_end);
        return (scheme == "http" || scheme == "https");
    }

    return false;
}

// Parse absolute URI (with scheme)
void Uri::parse_absolute_uri(const std::string& uri_string) {
    size_t scheme_end = uri_string.find("://");
    if (scheme_end == std::string::npos) {
        return;  // Not an absolute URI
    }

    // Extract scheme
    scheme_ = uri_string.substr(0, scheme_end);
    absolute_ = true;

    // Find start of authority component
    size_t authority_start = scheme_end + 3;  // Skip "://"

    // Find end of authority component (first '/' after authority)
    size_t path_start = uri_string.find('/', authority_start);

    // Extract authority (host:port)
    std::string authority;
    if (path_start != std::string::npos) {
        authority = uri_string.substr(authority_start, path_start - authority_start);
        // Extract path and query string
        path_ = uri_string.substr(path_start);
        extract_query_string(path_);
    } else {
        authority = uri_string.substr(authority_start);
        path_ = "/";  // Default path is root
    }

    // Process authority (host:port)
    extract_authority_components(authority);

    normalize_path();
}

// Extract host and port from authority component
void Uri::extract_authority_components(const std::string& authority) {
    // Check for port in authority
    size_t colon_pos = authority.find(':');

    if (colon_pos != std::string::npos) {
        host_ = authority.substr(0, colon_pos);
        std::string port_str = authority.substr(colon_pos + 1);

        // Parse port number
        try {
            port_ = std::atoi(port_str.c_str());
            if (port_ <= 0 || port_ > MAX_PORT_NUMBER) {
                port_ = (scheme_ == "https") ? HTTPS_DEFAULT_PORT
                                             : HTTP_DEFAULT_PORT;  // Default to standard ports
            }
        } catch (...) {
            port_ = (scheme_ == "https") ? HTTPS_DEFAULT_PORT
                                         : HTTP_DEFAULT_PORT;  // Default to standard ports
        }
    } else {
        host_ = authority;
        port_ = (scheme_ == "https") ? HTTPS_DEFAULT_PORT
                                     : HTTP_DEFAULT_PORT;  // Default to standard ports
    }
}

// Separates path and query string from URI
void Uri::extract_query_string(const std::string& uri) {
    size_t query_pos = uri.find('?');
    if (query_pos != std::string::npos) {
        query_string_ = uri.substr(query_pos + 1);
        path_ = uri.substr(0, query_pos);
    }
}

// Parses query string into key-value pairs
void Uri::parse_query_params() {
    query_params_.clear();
    if (query_string_.empty()) {
        return;
    }

    std::istringstream query_stream(query_string_);
    std::string pair;

    while (std::getline(query_stream, pair, '&')) {
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = pair.substr(0, eq_pos);
            std::string value = pair.substr(eq_pos + 1);
            query_params_[decode(key)] = decode(value);
        } else {
            // Parameter with no value
            query_params_[decode(pair)] = "";
        }
    }
}

// Extracts just the path part from a URI, removing any query string
std::string Uri::extract_path(const std::string& uri) {
    size_t query_pos = uri.find('?');
    if (query_pos != std::string::npos) {
        return uri.substr(0, query_pos);
    }
    return uri;
}
