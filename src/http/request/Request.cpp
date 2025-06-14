/**
 * @file Request.cpp
 * @brief Core HTTP request parsing and lifecycle management.
 *
 * @see RFC 7230 - HTTP/1.1 Message Syntax and Routing
 */

#include "Request.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

#include "../../utils/Log.hpp"
#include "../common/Headers.hpp"
#include "../common/Methods.hpp"
#include "../error/Error.hpp"

// ------------------------------------------------------------------
// Constructor & Destructor

HttpRequest::HttpRequest()
    : method_(HttpMethods::UNKNOWN),
      headers_parsed_(false),
      complete_(false),
      chunked_(false),
      current_chunk_size_(0),
      max_content_length_(DEFAULT_MAX_CONTENT_LENGTH),
      header_count_(0) {
}

HttpRequest::~HttpRequest() {
}

// ------------------------------------------------------------------
// Public Methods

void HttpRequest::reset() {
    method_ = HttpMethods::UNKNOWN;
    uri_ = Uri();
    http_version_.clear();
    headers_.clear();
    body_.clear();
    path_info_.clear();
    script_name_.clear();

    headers_parsed_ = false;
    complete_ = false;
    chunked_ = false;
    request_buffer_.clear();
    body_buffer_.clear();
    current_chunk_size_ = 0;
    header_count_ = 0;
    // Keep max_content_length_ as it might be configured externally
}

void HttpRequest::append_data(const std::string& data) {
    // Add new data to the buffer
    request_buffer_ += data;

    // If already complete, don't parse again
    if (complete_) {
        return;
    }

    // Attempt to parse with current buffer contents
    parse(request_buffer_);
}

// ------------------------------------------------------------------
// Parsing

void HttpRequest::parse(const std::string& raw_request) {
    // Check for empty request
    if (raw_request.empty()) {
        throw HttpError(BAD_REQUEST, "Empty request");
    }

    // If we already have a complete request, don't parse again
    if (complete_) {
        return;
    }

    // First, try to parse headers if they haven't been parsed yet
    if (!headers_parsed_) {
        // Check for complete headers
        size_t header_end = raw_request.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            return;  // Headers incomplete, need more data
        }

        // Split request into headers and body sections
        std::string headers_section = raw_request.substr(0, header_end);
        std::string remaining_data = raw_request.substr(header_end + 4);

        // Parse request line
        size_t first_line_end = headers_section.find("\r\n");
        if (first_line_end == std::string::npos) {
            throw HttpError(BAD_REQUEST, "Invalid request line");
        }

        std::string request_line = headers_section.substr(0, first_line_end);
        std::string headers_content = headers_section.substr(first_line_end + 2);

        parse_request_line(request_line);
        parse_headers(headers_content);
        validate_headers();

        headers_parsed_ = true;

        // If there's body data, update the request buffer to only contain the body
        if (!remaining_data.empty()) {
            request_buffer_ = remaining_data;
        } else {
            request_buffer_.clear();
        }
    }

    // If headers are parsed, process the body if needed
    if (headers_parsed_ && !complete_) {
        // Special cases for requests without body
        // RFC 7231: GET, HEAD, DELETE, OPTIONS, TRACE typically don't have bodies
        if (method_ == HttpMethods::GET || method_ == HttpMethods::DELETE ||
            method_ == HttpMethods::HEAD || method_ == HttpMethods::OPTIONS ||
            method_ == HttpMethods::TRACE) {
            complete_ = true;
            return;
        }

        // For other methods (POST, PUT, PATCH, etc.), check Content-Length
        if (method_ == HttpMethods::POST || method_ == HttpMethods::PUT ||
            method_ == HttpMethods::PATCH) {
            std::string content_length = HttpHeaders::get(headers_, HttpHeaders::CONTENT_LENGTH);
            std::string transfer_encoding =
                HttpHeaders::get(headers_, HttpHeaders::TRANSFER_ENCODING);

            bool has_content_length = !content_length.empty();
            bool has_chunked_encoding = (transfer_encoding.find("chunked") != std::string::npos);

            // RFC 7230: MUST return 411 if no Content-Length and no Transfer-Encoding
            if (!has_content_length && !has_chunked_encoding && !request_buffer_.empty()) {
                throw HttpError(LENGTH_REQUIRED, "Content-Length header required");
            }

            // If no Content-Length and no Transfer-Encoding, treat as complete with empty body
            if (!has_content_length && !has_chunked_encoding) {
                complete_ = true;
                return;
            }

            // Check for error 413 :
            if (has_content_length) {
                std::stringstream ss(content_length);
                size_t body_size;
                ss >> body_size;

                // Check if conversion failed
                if (ss.fail()) {
                    throw HttpError(BAD_REQUEST, "Invalid Content-Length");
                }

                // Check against maximum allowed size
                if (body_size > max_content_length_) {
                    throw HttpError(PAYLOAD_TOO_LARGE, "Request entity too large");
                }
            }

            // If Content-Length is 0, request is complete
            if (content_length == "0") {
                complete_ = true;
                return;
            }
        }

        // For any other methods (CONNECT, custom methods), assume no body
        if (method_ == HttpMethods::CONNECT || method_ == HttpMethods::UNKNOWN) {
            complete_ = true;
            return;
        }

        // Process any body data we have
        if (!request_buffer_.empty()) {
            parse_body(request_buffer_);
            // If using chunked encoding, keep the buffer for subsequent chunks
            // Otherwise clear it as it's been processed
            if (!chunked_ || complete_) {
                request_buffer_.clear();
            }
        }
    }
}

// ------------------------------------------------------------------
// Copy Constructor & Assignment

HttpRequest::HttpRequest(const HttpRequest& other)
    : method_(other.method_),
      uri_(other.uri_),
      http_version_(other.http_version_),
      headers_(other.headers_),
      body_(other.body_),
      path_info_(other.path_info_),
      script_name_(other.script_name_),
      headers_parsed_(other.headers_parsed_),
      complete_(other.complete_),
      chunked_(other.chunked_),
      request_buffer_(other.request_buffer_),
      body_buffer_(other.body_buffer_),
      current_chunk_size_(other.current_chunk_size_),
      max_content_length_(other.max_content_length_),
      header_count_(other.header_count_) {
}

HttpRequest& HttpRequest::operator=(const HttpRequest& other) {
    if (this != &other) {
        method_ = other.method_;
        uri_ = other.uri_;
        http_version_ = other.http_version_;
        headers_ = other.headers_;
        body_ = other.body_;
        path_info_ = other.path_info_;
        script_name_ = other.script_name_;
        headers_parsed_ = other.headers_parsed_;
        complete_ = other.complete_;
        chunked_ = other.chunked_;
        request_buffer_ = other.request_buffer_;
        body_buffer_ = other.body_buffer_;
        current_chunk_size_ = other.current_chunk_size_;
        max_content_length_ = other.max_content_length_;
        header_count_ = other.header_count_;
    }
    return *this;
}
