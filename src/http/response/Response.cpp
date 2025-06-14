#include "Response.hpp"

#include <ctime>
#include <iostream>
#include <sstream>

#include "../../utils/Log.hpp"
#include "../common/Headers.hpp"
#include "../error/Error.hpp"

HttpResponse::HttpResponse() : status_(OK) {
    // Add by default
    set_date_header();
    set_header(HttpHeaders::SERVER, "WebServ");
}

HttpResponse::~HttpResponse() {
}

void HttpResponse::set_status(HttpStatusCode status) {
    status_ = status;
}

void HttpResponse::set_header(const std::string& name, const std::string& value) {
    // Store headers with the normalized name format (e.g., "Content-Type")
    std::string normalized_name = HttpHeaders::normalize_name(name);

    // Use shared header handling logic
    HttpHeaders::add_header(headers_, normalized_name, value);
}

void HttpResponse::set_body(const std::string& body) {
    body_ = body;
    // Automatically set Content-Length when body is set - using HttpHeaders constant
    set_header(HttpHeaders::CONTENT_LENGTH, Log::to_string(body_.size()));
}

std::string HttpResponse::build() const {
    std::ostringstream response;

    // Status line
    response << "HTTP/1.1 " << status_ << " " << ::get_status_message(status_) << "\r\n";

    // Headers
    for (HeaderMapConstIt it = headers_.begin(); it != headers_.end(); ++it) {
        response << it->first << ": " << it->second << "\r\n";
    }

    // Empty line to separate headers from body
    response << "\r\n";

    // Add the body to the response
    response << body_;

    return response.str();
}

void HttpResponse::set_date_header() {
    char date_buf[100];
    time_t now = time(0);
    struct tm* tm_info = gmtime(&now);

    // Format according to HTTP spec
    strftime(date_buf, sizeof(date_buf), "%a, %d %b %Y %H:%M:%S GMT", tm_info);

    // Using HttpHeaders constant
    set_header(HttpHeaders::DATE, date_buf);
}

HttpResponse HttpResponse::build_default_error_response(const HttpError& error) {
    HttpResponse response;
    response.set_status(error.get_status_code());
    response.set_header(HttpHeaders::CONTENT_TYPE, "text/html");
    response.set_body(error.get_error_page());
    return response;
}

std::string HttpResponse::get_header(const std::string& name) const {
    // Get the first header matching the name (case insensitive)
    for (HeaderMapConstIt it = headers_.begin(); it != headers_.end(); ++it) {
        if (HttpHeaders::compare_insensitive(it->first, name)) {
            return it->second;
        }
    }
    return "";
}

std::string HttpResponse::get_body() const {
    return body_;
}
