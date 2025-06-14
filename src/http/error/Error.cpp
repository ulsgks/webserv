#include "Error.hpp"

#include <sstream>

#include "../common/StatusCode.hpp"

// HTTP error handling constants
static const int SERVER_ERROR_THRESHOLD = 500;  // Status codes >= 500 are server errors

HttpError::HttpError(HttpStatusCode status, const std::string& message)
    : std::runtime_error(message.empty() ? ::get_status_message(status) : message),
      status_code_(status) {
    generate_error_page();
}

HttpError::~HttpError() throw() {
}

HttpStatusCode HttpError::get_status_code() const {
    return status_code_;
}

const char* HttpError::get_status_message() const {
    return what();
}

std::string HttpError::get_error_page() const {
    return error_page_;
}

void HttpError::generate_error_page() {
    std::ostringstream ss;
    ss << "<!DOCTYPE html>\n"
       << "<html>\n"
       << "<head><title>Error</title></head>\n"
       << "<body>\n"
       << "<h1>" << status_code_ << " - " << ::get_status_message(status_code_) << "</h1>\n"
       << "</body>\n"
       << "</html>";
    error_page_ = ss.str();
}

bool HttpError::should_close_connection() const {
    // NGINX-like behavior: Close connection for serious protocol errors and server errors

    // All server errors (5xx) should close the connection
    if (status_code_ >= SERVER_ERROR_THRESHOLD) {
        return true;
    }

    // Specific client errors (4xx) that should close the connection
    switch (status_code_) {
        case BAD_REQUEST:             // 400: Malformed request
        case REQUEST_TIMEOUT:         // 408: Client took too long
        case LENGTH_REQUIRED:         // 411: Missing Content-Length
        case PAYLOAD_TOO_LARGE:       // 413: Request body too large
        case URI_TOO_LONG:            // 414: URI exceeds server limits
        case UNSUPPORTED_MEDIA_TYPE:  // 415: Unsupported content type
            return true;
        default:
            // For other client errors (404, 403, etc.), don't force close
            return false;
    }
}
