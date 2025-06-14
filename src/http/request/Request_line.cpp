/**
 * @file Request_line.cpp
 * @brief HTTP request line parsing and validation.
 *
 * @example "GET /index.html HTTP/1.1"
 * @example "GET http://example.com/index.html HTTP/1.1"
 *
 * @see RFC 7230, Section 3.1.1 (Request Line)
 * @see RFC 7230, Section 5.3 (Request Target)
 */

#include <sstream>

#include "../../utils/Log.hpp"
#include "../common/Methods.hpp"
#include "../error/Error.hpp"
#include "Request.hpp"

// HTTP port constants for Host header generation
static const int HTTP_STANDARD_PORT = 80;    // Standard HTTP port
static const int HTTPS_STANDARD_PORT = 443;  // Standard HTTPS port

void HttpRequest::parse_request_line(const std::string& line) {
    std::string method_str, uri_string, version;

    // Extract request line components
    extract_request_line_components(line, method_str, uri_string, version);

    // Validate components
    validate_method(method_str);
    validate_http_version(version);

    // Check for URI size limit BEFORE general validation for his specific error code
    if (!Uri::validate_uri_size(uri_string)) {
        throw HttpError(URI_TOO_LONG, "URI too long");
    }

    // Validate URI length and characters
    if (!Uri::validate_uri(uri_string)) {
        throw HttpError(BAD_REQUEST, "Invalid URI");
    }

    // Parse URI (handles both absolute and relative forms)
    // Note: Uri::parse() internally calls normalize_path() to handle path normalization
    // including removal of dot segments (./ and ../) as per RFC 3986
    uri_.parse(uri_string);

    // If URI is absolute, extract the host for Host header
    if (uri_.is_absolute()) {
        // Per RFC 7230, when an absolute URI is provided, set or override the Host header
        std::string host_value = uri_.get_host();
        if (uri_.get_port() != HTTP_STANDARD_PORT && uri_.get_port() != HTTPS_STANDARD_PORT) {
            host_value += ":" + HttpRequest::to_string(uri_.get_port());
        }
        HttpHeaders::add_header(headers_, "Host", host_value);
    }
}

void HttpRequest::extract_request_line_components(
    const std::string& line, std::string& method_str, std::string& uri_string,
    std::string& version) {
    std::istringstream iss(line);

    // Extract the three components
    if (!(iss >> method_str >> uri_string >> version)) {
        throw HttpError(BAD_REQUEST, "Malformed request line");
    }

    // Check for any extra components
    std::string extra;
    if (iss >> extra) {
        throw HttpError(BAD_REQUEST, "Extra components in request line");
    }
}

void HttpRequest::validate_method(const std::string& method_str) {
    // Normalize method to uppercase (HTTP methods are case-sensitive per RFC)
    std::string normalized_method = method_str;
    for (size_t i = 0; i < normalized_method.length(); ++i) {
        normalized_method[i] = std::toupper(normalized_method[i]);
    }

    // Convert to enum
    method_ = HttpMethods::from_string(normalized_method);

    // If it's a completely unknown method, return 501 Not Implemented
    if (method_ == HttpMethods::UNKNOWN) {
        throw HttpError(NOT_IMPLEMENTED, "Method not recognized: " + method_str);
    }

    // If it's a standard method we don't implement, we still store it
    // The handler will check if it's allowed for the specific resource
    // and return 405 Method Not Allowed if needed
}

void HttpRequest::validate_uri(const std::string& uri) {
    // This method is now used for compatibility and redirects to Uri::validate_uri
    if (!Uri::validate_uri(uri)) {
        throw HttpError(BAD_REQUEST, "Invalid URI format");
    }
}

void HttpRequest::validate_http_version(const std::string& version) {
    if (version != "HTTP/1.1" && version != "HTTP/1.0") {
        throw HttpError(HTTP_VERSION_NOT_SUPPORTED);
    }
    http_version_ = version;
}
