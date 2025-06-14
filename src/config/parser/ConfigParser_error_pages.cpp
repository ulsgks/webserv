// src/config/parser/ParserErrorPages.cpp
#include <cctype>

#include "ConfigParser.hpp"

void ConfigParser::parse_error_page(
    std::map<int, std::string>& error_pages, const std::vector<std::string>& values,
    const ConfigToken& directive_token) {
    if (values.size() < 2) {
        syntax_error("error_page requires at least two values", directive_token);
    }

    // The last value is the page path, check if it is not empty
    const std::string& page_path = values.back();
    if (page_path.empty()) {
        syntax_error("error_page path cannot be empty", directive_token);
    }

    // Parse all status codes (all values except the last one)
    for (size_t i = 0; i < values.size() - 1; ++i) {
        const std::string& status_str = values[i];

        // Check if the string contains only digits
        if (status_str.empty()) {
            syntax_error("Invalid HTTP status code: empty value", directive_token);
        }

        for (size_t j = 0; j < status_str.length(); ++j) {
            if (!std::isdigit(status_str[j])) {
                syntax_error("Invalid HTTP status code: " + status_str, directive_token);
            }
        }

        int status_code = std::atoi(status_str.c_str());
        if (status_code < 100 || status_code > 599) {
            syntax_error("Invalid HTTP status code: " + status_str, directive_token);
        }

        error_pages[status_code] = page_path;
    }
}

void ConfigParser::parse_error_page_server_directive(
    ServerBlock& server, const DirectiveValues& values, const ConfigToken& directive_token) {
    parse_error_page(server.error_pages, values, directive_token);
}

void ConfigParser::parse_error_page_location_directive(
    LocationBlock& location, const DirectiveValues& values, const ConfigToken& directive_token) {
    parse_error_page(location.error_pages, values, directive_token);
}
