/**
 * @file Request_body.cpp
 * @brief HTTP message body parsing for normal and chunked transfers.
 *
 * @example
 * // Chunked encoding example
 * 4\r\n        // Chunk size in hex
 * Wiki\r\n     // Chunk data
 * 5\r\n        // Next chunk size
 * pedia\r\n    // Next chunk data
 * 0\r\n        // Final chunk (size 0)
 * \r\n         // End of chunked body
 *
 * @see RFC 7230, Section 3.3 (Message Body)
 * @see RFC 7230, Section 4.1 (Chunked Transfer Coding)
 */

#include <cstdlib>

#include "../common/Headers.hpp"
#include "../error/Error.hpp"
#include "Request.hpp"

void HttpRequest::parse_body(const std::string& data) {
    if (chunked_) {
        parse_chunked_body(data);
    } else {
        parse_normal_body(data);
    }
}

void HttpRequest::parse_normal_body(const std::string& data) {
    // Using HttpHeaders utility instead of direct access
    std::string content_length_str = HttpHeaders::get(headers_, HttpHeaders::CONTENT_LENGTH);

    // Validate content length is present for POST requests
    if (method_ == HttpMethods::POST && content_length_str.empty()) {
        throw HttpError(BAD_REQUEST, "Missing Content-Length for POST request");
    }

    // If no content length and not POST, we're done
    if (content_length_str.empty()) {
        complete_ = true;
        return;
    }

    // Parse content length
    char* end;
    unsigned long content_length = std::strtoul(content_length_str.c_str(), &end, 10);
    if (*end != '\0') {
        throw HttpError(BAD_REQUEST, "Invalid Content-Length value");
    }

    // Check against maximum allowed size
    if (content_length > max_content_length_) {
        throw HttpError(PAYLOAD_TOO_LARGE);
    }

    // Append new data
    body_ += data;

    // Check if we've received all the data
    if (body_.length() >= content_length) {
        if (body_.length() > content_length) {
            // Truncate to exact content length
            body_ = body_.substr(0, content_length);
        }
        complete_ = true;
    }
}

void HttpRequest::parse_chunked_body(const std::string& data) {
    // Append new data to existing body buffer
    body_buffer_ += data;

    // Process chunks until we need more data or reach the end
    while (!body_buffer_.empty()) {
        // If we're not currently processing a chunk, look for chunk size
        if (current_chunk_size_ == 0) {
            size_t line_end = body_buffer_.find("\r\n");
            if (line_end == std::string::npos) {
                return;  // Need more data to find the chunk size line
            }

            std::string chunk_header = body_buffer_.substr(0, line_end);

            // Remove chunk extensions if present (separated by semicolon)
            size_t semicolon = chunk_header.find(';');
            if (semicolon != std::string::npos) {
                chunk_header = chunk_header.substr(0, semicolon);
            }

            // Trim any potential whitespace
            chunk_header = trim(chunk_header);

            // Parse chunk size (hex value)
            char* end;
            current_chunk_size_ = std::strtoul(chunk_header.c_str(), &end, 16);
            if (*end != '\0') {
                throw HttpError(BAD_REQUEST, "Invalid chunk size: " + chunk_header);
            }

            // Remove chunk header from buffer
            body_buffer_ = body_buffer_.substr(line_end + 2);

            // If chunk size is 0, this is the last chunk
            if (current_chunk_size_ == 0) {
                // Handle the end of chunked message
                process_final_chunk();
                return;
            }
        }

        // Process current chunk - do we have enough data?
        if (body_buffer_.length() < current_chunk_size_ + 2) {
            return;  // Need more data to complete this chunk
        }

        // We have enough data for this chunk - append to final body
        body_ += body_buffer_.substr(0, current_chunk_size_);

        // Remove chunk data and CRLF from buffer
        body_buffer_ = body_buffer_.substr(current_chunk_size_ + 2);

        // Reset current chunk size to look for the next chunk
        current_chunk_size_ = 0;
    }
}

bool HttpRequest::process_final_chunk() {
    // We've reached the zero-sized chunk
    // Now we need to handle the final CRLF or any trailing headers

    // Look for the terminating sequence (either just CRLF or CRLF CRLF for trailers)
    size_t end_sequence = body_buffer_.find("\r\n\r\n");

    // If we find CRLF CRLF, this indicates the end with potential trailers
    if (end_sequence != std::string::npos) {
        // If there's content before the double CRLF, it's trailer fields
        if (end_sequence > 0) {
            parse_headers(body_buffer_.substr(0, end_sequence));
        }

        // Request is complete - we've found the final boundary
        complete_ = true;
        return true;
    }

    // Check for the simple case - just a CRLF with nothing after
    if (body_buffer_ == "\r\n") {
        complete_ = true;
        return true;
    }

    // Check if the buffer ends with CRLF and has no other data
    if (body_buffer_.length() >= 2 && body_buffer_[body_buffer_.length() - 2] == '\r' &&
        body_buffer_[body_buffer_.length() - 1] == '\n' &&
        body_buffer_.find("\r\n") == body_buffer_.length() - 2) {
        // This is a complete message with just CRLF at the end
        complete_ = true;
        return true;
    }

    // If the buffer doesn't contain a complete trailer section or a final CRLF,
    // we need more data - return false
    return false;
}
