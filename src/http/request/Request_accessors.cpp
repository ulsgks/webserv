/**
 * @file Request_accessors.cpp
 * @brief Accessor methods for HTTP request components.
 */

// Request_accessors.cpp
#include "../common/Headers.hpp"
#include "Request.hpp"

// ------------------------------------------------------------------
// Accessor Methods

HttpMethods::Method HttpRequest::get_method() const {
    return method_;
}

const std::string& HttpRequest::get_path() const {
    return uri_.get_path();
}

const std::string& HttpRequest::get_query_string() const {
    return uri_.get_query_string();
}

const std::string& HttpRequest::get_http_version() const {
    return http_version_;
}

const std::string& HttpRequest::get_body() const {
    return body_;
}

const HeaderMap& HttpRequest::get_headers() const {
    return headers_;
}

bool HttpRequest::is_complete() const {
    return complete_;
}

bool HttpRequest::is_chunked() const {
    return chunked_;
}

bool HttpRequest::is_keep_alive() const {
    std::string connection = HttpHeaders::get(headers_, HttpHeaders::CONNECTION);

    if (http_version_ == "HTTP/1.1") {
        // In HTTP/1.1, connections are keep-alive by default
        return !HttpHeaders::value_contains(connection, "close");
    } else {
        // In HTTP/1.0, connections are closed by default
        return HttpHeaders::value_contains(connection, "keep-alive");
    }
}

std::string HttpRequest::get_header(const std::string& name) const {
    return HttpHeaders::get(headers_, name);
}

void HttpRequest::set_max_content_length(size_t length) {
    max_content_length_ = length;
}
