/**
 * @file Request_headers.cpp
 * @brief HTTP header parsing, validation, and storage with strict RFC 7230 compliance.
 *
 * @see RFC 7230, Section 3.2 (Header Fields)
 */

#include <sstream>

#include "../common/Headers.hpp"
#include "../error/Error.hpp"
#include "Request.hpp"

void HttpRequest::parse_headers(const std::string& headers_content) {
    std::istringstream iss(headers_content);
    std::string line;
    std::string current_header_name;
    std::string current_header_value;

    while (std::getline(iss, line)) {
        // Normalize line ending
        remove_carriage_return(line);

        // Skip empty lines
        if (line.empty()) {
            continue;
        }

        // Process the line based on its type
        if (is_header_continuation(line)) {
            // RFC 7230 Section 3.2.4: obs-fold is deprecated and MUST be rejected
            throw HttpError(BAD_REQUEST, "Obsolete line folding is deprecated");
        } else {
            // Store previous header if we have one
            store_pending_header(current_header_name, current_header_value);

            // Parse and validate the new header line
            parse_header_line(line, current_header_name, current_header_value);
        }
    }

    // Store the last header if present
    store_pending_header(current_header_name, current_header_value);
}

void HttpRequest::remove_carriage_return(std::string& line) {
    if (!line.empty() && line[line.length() - 1] == '\r') {
        line.erase(line.length() - 1);
    }
}

bool HttpRequest::is_header_continuation(const std::string& line) {
    return !line.empty() && (line[0] == ' ' || line[0] == '\t');
}

void HttpRequest::store_pending_header(std::string& name, std::string& value) {
    if (!name.empty()) {
        store_header(name, value);
        name.clear();
        value.clear();
    }
}

void HttpRequest::parse_header_line(
    const std::string& line, std::string& header_name, std::string& header_value) {
    // Find the first colon
    size_t colon = line.find(':');
    if (colon == std::string::npos) {
        throw HttpError(BAD_REQUEST, "Invalid header format");
    }

    // Check if the header line is malformed - RFC 7230 section 3.2.4
    if (is_malformed_header_line(line, colon)) {
        throw HttpError(BAD_REQUEST, "Malformed header line");
    }

    // Extract the field name and value
    header_name = line.substr(0, colon);
    std::string field_value = line.substr(colon + 1);

    // Validate and trim whitespace from field value
    header_value = trim(field_value);

    // Validate header name and value
    validate_header_name(header_name);
    validate_header_value(header_value);
}

bool HttpRequest::is_malformed_header_line(const std::string& line, size_t first_colon) {
    // RFC 7230, Section 3.2.4: No whitespace is allowed between field-name and colon
    if (has_whitespace_before_colon(line, first_colon)) {
        return true;
    }

    // Check for consecutive colons (e.g., "Header:: Value") which is malformed
    if (first_colon + 1 < line.length() && line[first_colon + 1] == ':') {
        return true;
    }

    // Check if there's any colon in the field-name before the first_colon
    for (size_t i = 0; i < first_colon; ++i) {
        if (line[i] == ':') {
            return true;
        }
    }

    return false;
}

bool HttpRequest::has_whitespace_before_colon(const std::string& line, size_t colon_pos) {
    // Check for whitespace immediately before the colon
    if (colon_pos > 0) {
        return (line[colon_pos - 1] == ' ' || line[colon_pos - 1] == '\t');
    }
    return false;
}

void HttpRequest::validate_header_name(const std::string& name) {
    // RFC 7230, Section 3.2.6: field-name token validation
    if (name.empty()) {
        throw HttpError(BAD_REQUEST, "Empty header name");
    }

    for (size_t i = 0; i < name.length(); ++i) {
        char c = name[i];
        if (!is_token_char(c)) {
            throw HttpError(BAD_REQUEST, "Invalid character in header name");
        }
    }
}

bool HttpRequest::is_token_char(char c) {
    // RFC 7230, Section 3.2.6: token = 1*tchar
    return (c >= '0' && c <= '9') ||  // DIGIT
           (c >= 'a' && c <= 'z') ||  // ALPHA lower
           (c >= 'A' && c <= 'Z') ||  // ALPHA upper
           c == '!' || c == '#' || c == '$' || c == '%' || c == '&' || c == '\'' || c == '*' ||
           c == '+' || c == '-' || c == '.' || c == '^' || c == '_' || c == '`' || c == '|' ||
           c == '~';
}

void HttpRequest::validate_header_value(const std::string& value) {
    // Check header value size limit (RFC 7230 recommends reasonable limits)
    if (value.length() > MAX_HEADER_SIZE) {
        throw HttpError(REQUEST_HEADER_FIELDS_TOO_LARGE, "Header value too large");
    }

    // Validate the character set (RFC 7230, Section 3.2)
    for (size_t i = 0; i < value.length(); ++i) {
        unsigned char c = value[i];

        // Only allow visible ASCII chars, spaces, tabs, or obsolete text
        if ((c < 0x20 && c != 0x09) || c == 0x7F) {
            throw HttpError(BAD_REQUEST, "Invalid control character in header value");
        }
    }
}

std::string HttpRequest::trim(const std::string& str) {
    // RFC 7230, Section 3.2.4: OWS (optional whitespace)
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos) {
        return "";  // All whitespace is treated as an empty value
    }
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, last - first + 1);
}

void HttpRequest::store_header(const std::string& name, const std::string& value) {
    // Check if we've reached the maximum number of headers
    if (header_count_ >= MAX_HEADERS) {
        throw HttpError(REQUEST_HEADER_FIELDS_TOO_LARGE, "Too many headers");
    }

    // Use RFC 7230 Section 3.2.2 compliant header handling
    HttpHeaders::add_header(headers_, name, value);
    header_count_++;

    // Special handling for specific headers - using the case-insensitive comparison
    if (HttpHeaders::compare_insensitive(name, HttpHeaders::TRANSFER_ENCODING)) {
        chunked_ = HttpHeaders::value_contains(value, "chunked");
    }
}

void HttpRequest::validate_headers() {
    // Check for required headers in HTTP/1.1
    if (http_version_ == "HTTP/1.1") {
        if (!HttpHeaders::has(headers_, HttpHeaders::HOST)) {
            throw HttpError(BAD_REQUEST, "HTTP/1.1 requires Host header");
        }
    }

    // Validate Content-Length if present
    std::string content_length = HttpHeaders::get(headers_, HttpHeaders::CONTENT_LENGTH);
    if (!content_length.empty()) {
        if (!HttpHeaders::is_valid_content_length(content_length)) {
            throw HttpError(BAD_REQUEST, "Invalid Content-Length value");
        }

        // Check max size
        char* end;
        unsigned long length = std::strtoul(content_length.c_str(), &end, 10);
        if (length > max_content_length_) {
            throw HttpError(PAYLOAD_TOO_LARGE);
        }
    }

    // Validate Transfer-Encoding
    std::string transfer_encoding = HttpHeaders::get(headers_, HttpHeaders::TRANSFER_ENCODING);
    if (!transfer_encoding.empty()) {
        if (HttpHeaders::value_contains(transfer_encoding, "chunked")) {
            chunked_ = true;

            // RFC 7230, Section 3.3.3: Content-Length must be ignored if Transfer-Encoding is
            // chunked
            if (!content_length.empty()) {
                throw HttpError(
                    BAD_REQUEST,
                    "Content-Length and chunked Transfer-Encoding cannot be used together");
            }
        }
    }
}
