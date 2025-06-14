// CgiResponse.hpp
#ifndef CGI_RESPONSE_HPP
#define CGI_RESPONSE_HPP

#include <cstdlib>
#include <map>
#include <sstream>
#include <string>

#include "../http/common/Headers.hpp"
#include "../http/common/StatusCode.hpp"
#include "../http/response/Response.hpp"
#include "../utils/Log.hpp"
#include "../utils/Types.hpp"

/**
 * Handles CGI response parsing and HTTP response building.
 *
 * This namespace provides functions for:
 * - Parsing CGI output (headers and body)
 * - Processing Status header
 * - Converting CGI headers to HTTP headers
 * - Building the final HTTP response
 */
namespace CgiResponse {

    inline HttpResponse build_from_output(const std::string& cgi_output);

    inline void parse_cgi_output(
        const std::string& cgi_output, HeaderMap& headers, std::string& body);

    inline HttpResponse build_response(const HeaderMap& headers, const std::string& body);

    // Implementation

    inline HttpResponse build_from_output(const std::string& cgi_output) {
        HeaderMap headers;
        std::string body;

        // Parse CGI output
        parse_cgi_output(cgi_output, headers, body);

        // Build and return HTTP response
        return build_response(headers, body);
    }

    inline void parse_cgi_output(
        const std::string& cgi_output, std::multimap<std::string, std::string>& headers,
        std::string& body) {
        // Find the header/body separator (blank line)
        size_t header_end = cgi_output.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            // Try Unix line endings
            header_end = cgi_output.find("\n\n");
            if (header_end == std::string::npos) {
                // No headers, entire output is body
                body = cgi_output;
                return;
            }
            header_end += 2;  // Skip the two newlines
        } else {
            header_end += 4;  // Skip \r\n\r\n
        }

        // Parse headers
        std::string header_section = cgi_output.substr(0, header_end);
        std::istringstream header_stream(header_section);
        std::string line;

        while (std::getline(header_stream, line)) {
            // Remove trailing \r if present
            if (!line.empty() && line[line.length() - 1] == '\r') {
                line.erase(line.length() - 1);
            }

            // Skip empty lines
            if (line.empty())
                continue;

            // Parse header line
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string name = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);

                // Trim leading whitespace from value
                size_t value_start = value.find_first_not_of(" \t");
                if (value_start != std::string::npos) {
                    value = value.substr(value_start);
                }

                // Use shared header handling logic
                HttpHeaders::add_header(headers, name, value);
            }
        }

        // Extract body
        if (header_end < cgi_output.length()) {
            body = cgi_output.substr(header_end);
        }
    }

    inline HttpResponse build_response(
        const std::multimap<std::string, std::string>& headers, const std::string& body) {
        HttpResponse response;

        // Default status
        HttpStatusCode status = OK;

        // Process CGI headers
        for (HeaderMapConstIt it = headers.begin(); it != headers.end(); ++it) {
            if (it->first == "Status") {
                // Parse status code from "Status: 404 Not Found" format
                int code = std::atoi(it->second.c_str());
                if (code >= 100 && code <= 599) {
                    status = static_cast<HttpStatusCode>(code);
                }
            } else {
                // Add other headers to response
                response.set_header(it->first, it->second);
            }
        }

        // Set status and body
        response.set_status(status);
        response.set_body(body);

        // If no Content-Type was set by CGI, default to text/html
        if (response.get_header(HttpHeaders::CONTENT_TYPE).empty()) {
            response.set_header(HttpHeaders::CONTENT_TYPE, "text/html");
        }

        return response;
    }
}  // namespace CgiResponse

#endif  // CGI_RESPONSE_HPP
